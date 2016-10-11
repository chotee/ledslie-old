import time

from twisted.internet import reactor
from twisted.internet.task import LoopingCall


class ConnectionWatchdog(object):
    """
    I help keeping watch over the senders connected to a zmq socket. I send them packets at regular intervals.
    """
    def __init__(self, connection, ):
        """
        Constructor
        :param connection: The connection of which senders need to be monitored.
        :type connection: Connection
        """
        self.connection = connection
        self.beat_frequency = 1.0  # every second
        self.setup_wait = 0.1 # seconds to wait before sending first heartbeat.
        self.allowed_beat_misses = 2  # How many beats to miss before flagging disconnect.
        self.grace_time = self.beat_frequency * self.allowed_beat_misses
        self.monitors = {}
        self.last_seen = {}
        self.actives = set()

    def start_monitor(self, remote_identity):
        """
        I start the monitoring of a sender with remote_identity.
        :param remote_identity: The Identity of the sender.
        :type remote_identity: str
        """
        self.last_seen[remote_identity] = time.time()
        if remote_identity not in self.monitors:
            self.monitors[remote_identity] = {
                'review': LoopingCall(self._review, remote_identity),
                'beat': LoopingCall(self._send_beat, remote_identity),
                'count': 0
            }
            self._start_checkloops(remote_identity)

    def _start_checkloops(self, remote_identity):
        beat = self.monitors[remote_identity]['beat']
        review = self.monitors[remote_identity]['review']
        def _start_beats(beat):
            if not beat.running:
                beat.start(self.beat_frequency)
        reactor.callLater(self.setup_wait, _start_beats,  beat)
        if not review.running:
            review.start(self.grace_time)

    def _send_beat(self, remote_identity):
        """
        I send a single beat message to the sender with remote_identity via the self.connection object.
        :param remote_identity: The remote identity to send a beat to.
        :type remote_identity: str
        """
        count = self.monitors[remote_identity]['count']
        #log.msg("Sending beat %d to remote identity %s" % (count, remote_identity))
        self.connection.sendMultipart(remote_identity, [b"ping", str(count).encode()])
        self.monitors[remote_identity]['count'] += 1

    def _review(self, remote_identity):
        """
        I periodically review the state of a sender via a connection. If the connection is stale I will call
        sender_disconnected on self.connection.
        :param remote_identity: The identity to review.
        :type remote_identity: str
        """
        last_seen = self.last_seen[remote_identity]
        if last_seen + self.grace_time > time.time():
            return  # Still active.
        if remote_identity in self.actives:
            self.actives.remove(remote_identity)
            self.monitors[remote_identity]['beat'].stop()
            self.monitors[remote_identity]['review'].stop()
            self.connection.sender_disconnected(remote_identity)

    def report_activity(self, remote_identity):
        """
        I get called whenever there's activity of a sender.
        :param remote_identity: ID of the sender that had activity
        :type remote_identity: str
        """
        #log.msg("%s is alive" % remote_identity)
        self.last_seen[remote_identity] = time.time()
        if remote_identity not in self.actives:
            self.actives.add(remote_identity)
            self._start_checkloops(remote_identity)
            self.connection.sender_established(remote_identity)