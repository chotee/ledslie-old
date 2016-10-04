from twisted.python import log

class SwitchboardService(object):
    def __init__(self):
        log.msg("Create %s" % self.__class__.__name__)
        self._roles = {}
        self._stats = {}

    def got_register(self, client, role):
        client.role = role
        self._roles.setdefault(role, []).append(client)
        return "Okay"

    def got_forward(self, client, for_role, message):
        key = (client.role, for_role)
        self._stats[key] = self._stats.get(key, 0) + 1
        for client in self._roles.get(for_role, []):
            client.send_message(message)

    def got_stats(self, client):
        summary = ["Source, Dest, Count"]
        for src, dst in self._stats:
            summary.append(", ".join([src, dst, str(self._stats[src, dst])]))
        return "\n".join(summary)
