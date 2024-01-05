from datetime import datetime
from aprslib.packets.base import APRSPacket
from aprslib.util import latitude_to_ddm, longitude_to_ddm, comment_altitude


class PositionReport(APRSPacket):
    format = 'uncompressed'

    _latitude = 0
    @property
    def latitude(self):
        return self._latitude

    @latitude.setter
    def latitude(self, val):
        if -90 <= val <= 90:
            self._latitude = val
        else:
            raise ValueError("Latitude outside of -90 to 90 degree range")

    _longitude = 0
    @property
    def longitude(self):
        return self._longitude

    @longitude.setter
    def longitude(self, val):
        if -180 <= val <= 180:
            self._longitude = val
        else:
            raise ValueError("Longitude outside of -180 to 180 degree range")

    symbol_table = '/'
    symbol = 'l'
    altitude = None
    timestamp = None
    comment = ''

    def _serialize_body(self):

        if self.timestamp is None:
            timestamp = ''
        elif isinstance(self.timestamp, str):
            timestamp = self.timestamp
        else:
            timestamp = datetime.utcfromtimestamp(self.timestamp).strftime("%d%H%M") + 'z'

        body = [
            '/' if self.timestamp else '!',  # packet type
            timestamp,
            latitude_to_ddm(self.latitude),
            self.symbol_table,
            longitude_to_ddm(self.longitude),
            self.symbol,
            comment_altitude(self.altitude) if self.altitude is not None else '',
            self.comment,
        ]

        return "".join(body)



