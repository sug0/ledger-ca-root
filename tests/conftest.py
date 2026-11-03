"""
Test setup: launch the application under test (speculos emulator or a real
device over USB) and provide the on-device review approval.

The backend is always explicit (--backend): "speculos" drives the review
over the emulator's REST API, "device" leaves the approval to the user on
the real device. Speculos needs qemu-arm-static (Linux; on macOS use the
Docker image), see tests/README.md for both setups.
"""

import os
import sys
import time
from pathlib import Path

import pytest
from speculos.client import SpeculosClient

from application_client import CaCommandSender
from application_client.device_client import DeviceClient

REPO_ROOT = Path(__file__).resolve().parents[1]
SCREENSHOT_DIR = Path(__file__).parent / "screenshots"

# Speculos seed the tests rely on (deterministic CA root key derivation)
SEED = "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about"

DEVICE_TARGETS = {  # speculos model -> build tree directory
    "nanosp": "nanos2",
}

REVIEW_TIMEOUT_S = 20
MAX_REVIEW_CLICKS = 6  # upper bound of review pages for the 3 pairs
HOLD_TIME_S = 1.2  # NBGL "hold to sign" duration


def console(message):
    # pytest captures stdout; write to the real terminal so the user sees
    # the instructions while the test is waiting on the review
    print(message, file=sys.__stdout__, flush=True)


def pytest_addoption(parser):
    parser.addoption(
        "--backend",
        choices=["speculos", "device"],
        required=True,
        help="where the application runs: speculos emulator or a real device over USB HID",
    )
    parser.addoption(
        "--variant",
        choices=["ed25519", "p256"],
        default=os.environ.get("CA_VARIANT", "ed25519"),
        help="CA key curve variant to test (build tree build/<variant>)",
    )
    parser.addoption(
        "--model",
        choices=sorted(DEVICE_TARGETS.keys()),
        default="nanosp",
        help="speculos device model",
    )
    parser.addoption(
        "--approve-timeout",
        type=int,
        default=120,
        help="seconds to wait for the approval on the device (device backend)",
    )


@pytest.fixture(scope="session")
def backend(request):
    return request.config.getoption("--backend")


@pytest.fixture(scope="session")
def variant(request):
    return request.config.getoption("--variant")


@pytest.fixture(scope="session")
def app_elf(request, backend, variant):
    if backend == "device":
        # the application must already be loaded and open on the device
        return None
    device = DEVICE_TARGETS[request.config.getoption("--model")]
    elf = REPO_ROOT / "build" / variant / device / "bin" / "app.elf"
    if not elf.is_file():
        pytest.fail(f"application not built: {elf} is missing (run `make {variant}`)")
    return str(elf)


@pytest.fixture(scope="session")
def client(request, backend, app_elf):
    if backend == "device":
        device = DeviceClient(request.config.getoption("--approve-timeout"))
        device.open()
        yield device
        device.close()
        return

    args = [
        "--model", request.config.getoption("--model"),
        "--apiLevel", "27",
        "--seed", SEED,
    ]
    with SpeculosClient(app_elf, args) as speculos:
        yield speculos


@pytest.fixture(scope="session")
def sender(client):
    return CaCommandSender(client)


@pytest.fixture(scope="session")
def review_approve(client, backend):
    """Approve the NBGL certificate review.

    speculos: navigate to the hold page and hold the right button via the
    REST API (screens are dumped to tests/screenshots/). device: the user
    reviews and approves; the blocking SIGN_TBS exchange continues when
    they hold to sign.
    """
    if backend == "device":

        def approve():
            console(">>> On the device: review the certificate, then HOLD the right button to sign.")
            console(">>> (a long press on the left button rejects, failing the test)")

        return approve

    def approve():
        SCREENSHOT_DIR.mkdir(exist_ok=True)
        (SCREENSHOT_DIR / "review.png").write_bytes(client.get_screenshot())

        deadline = time.monotonic() + REVIEW_TIMEOUT_S
        clicks = 0
        while time.monotonic() < deadline and clicks < MAX_REVIEW_CLICKS:
            texts = _screen_texts(client)
            if any("hold to sign" in text.lower() for text in texts):
                break
            client.press_and_release("right")
            clicks += 1
            time.sleep(0.3)
        (SCREENSHOT_DIR / "review-hold.png").write_bytes(client.get_screenshot())

        # hold the right button to sign; the APDU response is sent on release
        _press(client, "right", "press")
        time.sleep(HOLD_TIME_S)
        _press(client, "right", "release")
        time.sleep(0.3)

    return approve


def _press(client, button, action):
    client.session.post(f"{client.api_url}/button/{button}", json={"action": action})


def _screen_texts(client):
    """Texts currently on screen; empty when the extraction is unavailable."""
    try:
        content = client.get_current_screen_content()
        return content.get("text") or []
    except Exception:
        return []
