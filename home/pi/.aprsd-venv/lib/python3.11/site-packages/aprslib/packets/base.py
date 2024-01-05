from aprslib import parse
from aprslib.parsing.common import parse_header

class APRSPacket(object):
    format = 'raw'
    fromcall = 'N0CALL'
    tocall = 'N0CALL'
    path = []

    def __init__(self, data=None):
        if data:
            self.load(data)

    def __repr__(self):
        return "<%s(%s)>" % (
                    self.__class__.__name__,
                    repr(str(self)),
                    )

    def __str__(self):
        return "%s:%s" % (
            self._serialize_header(),
            self._serialize_body(),
            )

    def __eq__(self, other):
        return str(self) == str(other)

    def _serialize_header(self):
        header = "%s>%s" % (self.fromcall, self.tocall)

        if self.path:
            header += "," + ",".join(self.path)

        return header

    def _serialize_body(self):
        return getattr(self, 'body', '')


    def load(self, obj):
        if not isinstance(obj, dict):
            if self.format == 'raw':
                header, self.body = obj.split(":", 1)
                obj = parse_header(header)
            else:
                obj = parse(obj)

        for k, v in obj.items():
            if k == 'format':
                continue
            if k == 'to' or k == 'from':
                k += 'call'

            if hasattr(self, k):
                setattr(self, k, v)
