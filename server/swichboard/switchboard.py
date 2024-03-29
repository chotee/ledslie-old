from twisted.python import log
from twisted.internet import task


class SwitchboardService(object):
    def __init__(self, rolodex):
        log.msg("Create %s" % self.__class__.__name__)
        self._rolodex = rolodex
        self._roles = {}
        self._stats = {}

    def got_send_second(self, message):
        log.msg("got_send_second %s" % message)

    def req_reverse(self, client, request):
        return "reversed", request[::-1]

    def got_register(self, client, role):
        client.role = role
        self._roles.setdefault(role, []).append(client)
        return "Okay"

    def got_forward(self, client, for_role, message):
        key = (client.role, for_role)
        self._stats[key] = self._stats.get(key, 0) + 1
        for client in self._roles.get(for_role, []):
            client.send_message(message)

    # def got_stats(self, client):
    #     summary = ["Source, Dest, Count"]
    #     for src, dst in self._stats:
    #         summary.append(", ".join([src, dst, str(self._stats[src, dst])]))
    #     return "\n".join(summary)

    # def time_passes(self):
    #     clients_list = set()
    #     for role_client in self._roles.values():
    #         clients_list.update(role_client)
    #     for client in clients_list:
    #         client.send_message("time's a wastin'")
    #     log.msg("Time has passed")