"""
HID transport to drive the tests against a real device over USB.

Mirrors the subset of the speculos client interface used by the test
(apdu_exchange, apdu_exchange_nowait + receive) so the APDU command sender
stays transport-agnostic. On a real device everything is driven by the host
except the review approval, which the user performs physically; the exchange
for the last SIGN_TBS chunk blocks until they do (or until the timeout).
"""

from contextlib import contextmanager
from threading import Thread

# APDU framing per doc/APDU.md: 1-byte Lc, no Le field
MAX_APDU_DATA = 255


class ApduError(Exception):
    """The device answered with a status word other than 9000."""

    def __init__(self, sw, data=b""):
        self.sw = sw
        self.data = data
        super().__init__(f"device returned status word {sw:04x}")


def _exchange(dongle, apdu, timeout_s):
    # lazy import: keeps the speculos-only test environments working without
    # ledgerblue installed (CI)
    from ledgerblue.commException import CommException

    try:
        # success responses come back without the status word
        return dongle.exchange(apdu, timeout=timeout_s)
    except CommException as exc:
        raise ApduError(exc.sw, exc.data or b"") from exc


class DeviceResponse:
    """A blocking exchange started in a background thread.

    receive() joins the thread, bounding the wait for the user's on-device
    approval; exchange errors are re-raised on the joining side.
    """

    def __init__(self, dongle, apdu, timeout_s):
        self._dongle = dongle
        self._apdu = apdu
        self._timeout_s = timeout_s
        self._thread = Thread(target=self._exchange, daemon=True)
        self._data = None
        self._error = None

    def _exchange(self):
        try:
            self._data = _exchange(self._dongle, self._apdu, self._timeout_s)
        except Exception as exc:  # ApduError or transport failure
            self._error = exc

    def start(self):
        self._thread.start()

    def receive(self, timeout=None):
        self._thread.join(timeout if timeout is not None else self._timeout_s)
        if self._thread.is_alive():
            raise TimeoutError(
                f"no response from the device within {self._timeout_s} s"
                " (was the request approved on the device?)"
            )
        if self._error is not None:
            raise self._error
        return self._data


class DeviceClient:
    """APDU exchange with the application running on a real device."""

    backend = "device"

    def __init__(self, timeout_s):
        self.timeout_s = timeout_s
        self._dongle = None

    def open(self):
        from ledgerblue.comm import getDongle

        self._dongle = getDongle()
        return self

    def close(self):
        if self._dongle is not None:
            self._dongle.close()
            self._dongle = None

    @staticmethod
    def _apdu_bytes(cla, ins, data, p1, p2):
        if len(data) > MAX_APDU_DATA:
            raise ValueError(f"APDU data over {MAX_APDU_DATA} bytes")
        return bytes((cla, ins, p1, p2, len(data))) + data

    def apdu_exchange(self, cla, ins, data=b"", p1=0, p2=0):
        apdu = self._apdu_bytes(cla, ins, data, p1, p2)
        return _exchange(self._dongle, apdu, self.timeout_s)

    @contextmanager
    def apdu_exchange_nowait(self, cla, ins, data=b"", p1=0, p2=0):
        apdu = self._apdu_bytes(cla, ins, data, p1, p2)
        response = DeviceResponse(self._dongle, apdu, self.timeout_s)
        response.start()
        yield response
