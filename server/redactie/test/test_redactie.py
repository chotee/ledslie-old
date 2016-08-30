def setUp(self):
    factory = RemoteCalculationFactory()
    self.proto = factory.buildProtocol(('127.0.0.1', 0))
    self.tr = proto_helpers.StringTransport()
    self.proto.makeConnection(self.tr)