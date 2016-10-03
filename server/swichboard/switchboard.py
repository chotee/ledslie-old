
class SwitchboardService(object):
    def __init__(self):
        self._roles = {}

    def got_register(self, role, client):
        self._roles.setdefault(role, []).append(client)
        return "Okay"

    def got_forward(self, for_role, message):
        for client in self._roles.get(for_role, []):
            client.send_message(message)
