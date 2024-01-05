"""Network parameter types"""
import ipaddress

from .base import BaseParamType, ListParamType, RangeParamType


class IpAddress(BaseParamType):
    name = 'ip address'

    def __init__(self):
        super().__init__(_type=ipaddress.ip_address, errors=ValueError)


class IpAddressListParamType(ListParamType):
    name = 'ip address list'

    def __init__(self, separator: str = ',', ignore_empty: bool = False):
        super().__init__(IP_ADDRESS, separator=separator, name='ip addresses', ignore_empty=ignore_empty)


class Ipv4Address(BaseParamType):
    name = 'ipv4 address'

    def __init__(self):
        super().__init__(_type=ipaddress.IPv4Address, errors=ValueError)


class Ipv4AddressRange(RangeParamType):
    name = 'ipv4 address range'

    def __init__(
        self, minimum: ipaddress.IPv4Address = None, maximum: ipaddress.IPv4Address = None, clamp: bool = False
    ):
        super().__init__(Ipv4Address(), minimum, maximum, clamp)

    def __repr__(self):
        return f'IPV4AddressRange({repr(self._minimum)}, {repr(self._maximum)})'


class Ipv4AddressListParamType(ListParamType):
    name = 'ipv4 address list'

    def __init__(self, separator: str = ',', ignore_empty: bool = False):
        super().__init__(IPV4_ADDRESS, separator=separator, name='ipv4 addresses', ignore_empty=ignore_empty)


class Ipv6Address(BaseParamType):
    name = 'ipv6 address'

    def __init__(self):
        super().__init__(_type=ipaddress.IPv6Address, errors=ValueError)


class Ipv6AddressRange(RangeParamType):
    name = 'ipv6 address range'

    def __init__(
        self, minimum: ipaddress.IPv6Address = None, maximum: ipaddress.IPv6Address = None, clamp: bool = False
    ):
        super().__init__(Ipv6Address(), minimum, maximum, clamp)

    def __repr__(self):
        return f'IPV6AddressRange({repr(self._minimum)}, {repr(self._maximum)})'


class Ipv6AddressListParamType(ListParamType):
    name = 'ipv6 address list'

    def __init__(self, separator: str = ',', ignore_empty: bool = False):
        super().__init__(IPV6_ADDRESS, separator=separator, name='ipv6 addresses', ignore_empty=ignore_empty)


class IpNetwork(BaseParamType):
    name = 'ip network'

    def __init__(self):
        super().__init__(_type=ipaddress.ip_network, errors=ValueError)


class IpNetworkListParamType(ListParamType):
    name = 'ip network list'

    def __init__(self, separator: str = ',', ignore_empty: bool = False):
        super().__init__(IP_NETWORK, separator=separator, name='ip networks', ignore_empty=ignore_empty)


class Ipv4Network(BaseParamType):
    name = 'ipv4 network'

    def __init__(self):
        super().__init__(_type=ipaddress.IPv4Network, errors=ValueError)


class Ipv4NetworkListParamType(ListParamType):
    name = 'ipv4 network list'

    def __init__(self, separator: str = ',', ignore_empty: bool = False):
        super().__init__(IPV4_NETWORK, separator=separator, name='ipv4 networks', ignore_empty=ignore_empty)


class Ipv6Network(BaseParamType):
    name = 'ipv6 network'

    def __init__(self):
        super().__init__(_type=ipaddress.IPv6Network, errors=ValueError)


class Ipv6NetworkListParamType(ListParamType):
    name = 'ipv6 network list'

    def __init__(self, separator: str = ',', ignore_empty: bool = False):
        super().__init__(IPV6_NETWORK, separator=separator, name='ipv6 networks', ignore_empty=ignore_empty)


IP_ADDRESS = IpAddress()
IPV4_ADDRESS = Ipv4Address()
IPV6_ADDRESS = Ipv6Address()
IP_NETWORK = IpNetwork()
IPV4_NETWORK = Ipv4Network()
IPV6_NETWORK = Ipv6Network()
