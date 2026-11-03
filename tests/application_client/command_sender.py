"""
APDU client for the CA Root application, on top of the speculos client.

The application receives the TBSCertificate DER in chunks (P1 = strict
sequence number starting at 0, P2 = P2_MORE/P2_LAST) and answers the last
chunk only after the on-device review is approved.
"""

from enum import IntEnum

CLA = 0xE0
CHUNK_SIZE = 240  # Lc is capped at 255 by the APDU format


class INS(IntEnum):
    GET_VERSION = 0x03
    GET_APP_NAME = 0x04
    GET_CA_PUBLIC_KEY = 0x05
    SIGN_TBS = 0x06


P2_LAST = 0x00
P2_MORE = 0x80


class CaCommandSender:
    def __init__(self, client) -> None:
        # speculos.client.SpeculosClient
        self.client = client

    def get_version(self) -> bytes:
        return self.client.apdu_exchange(CLA, INS.GET_VERSION)

    def get_app_name(self) -> bytes:
        return self.client.apdu_exchange(CLA, INS.GET_APP_NAME)

    def get_ca_public_key(self) -> bytes:
        response = self.client.apdu_exchange(CLA, INS.GET_CA_PUBLIC_KEY)
        if len(response) < 1 or response[0] != len(response) - 1:
            raise ValueError(f"malformed public key response: {response.hex()}")
        return response[1:]

    def sign_tbs(self, tbs: bytes, on_review) -> bytes:
        """Send the TBSCertificate in chunks; on_review() drives the approval.

        Returns the raw signature produced over the exact tbs bytes.
        """
        chunks = [tbs[i:i + CHUNK_SIZE] for i in range(0, len(tbs), CHUNK_SIZE)]
        for number, chunk in enumerate(chunks[:-1]):
            self.client.apdu_exchange(CLA, INS.SIGN_TBS, chunk, p1=number, p2=P2_MORE)

        # the last chunk triggers the review; the answer arrives on approval
        with self.client.apdu_exchange_nowait(
            CLA, INS.SIGN_TBS, chunks[-1], p1=len(chunks) - 1, p2=P2_LAST
        ) as response:
            on_review()
            data = response.receive()

        if len(data) < 1 or data[0] != len(data) - 1:
            raise ValueError(f"malformed signature response: {data.hex()}")
        return data[1:]
