class SquidError(Exception):
    pass


class TimeoutError(SquidError):
    pass


class ChecksumError(SquidError):
    pass


class ProtocolError(SquidError):
    def __init__(self, error_code: int, message: str = ""):
        self.error_code = error_code
        super().__init__(f"Protocol error 0x{error_code:02X}: {message}")
