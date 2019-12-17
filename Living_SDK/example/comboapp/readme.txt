
1.make:
    For platform using vendor's ble stack, please add veriable btstack as follows, take bk7231u as example:
    aos make clean
    aos make comboapp@bk7231udevkitc btstack=vendor

2.set device&product info use cli command:
    linkkey ProductKey DeviceName DeviceSecret ProductSecret ProductID

3.smart config provision enter cli command:
    awss
    active_awss

4.ble awss provision enter cli command:
    ble_awss

5.reset wifi provision AP info and binding relations cli command:
    reset