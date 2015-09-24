from done.marg import Marg
from backend.models import Connection, DeliveryCenter, GraphNode


class Graph:
    def __init__(self, wbn):
        self.wbn = wbn
        connections = Connection.all()
        connections = [{
            'id': connection._id,
            'cutoff_departure': connection.departure.value,
            'cutoff_arrival': (
                connection.departure.value +
                connection.duration.value
            ),
            'origin': connection.origin.code,
            'destination': connection.destination.code
        } for connection in Connection.find({})]
        self.marg = Marg(connections)

    def handle_outscan(
            self, active, location, destination, scan_datetime, connection
    ):

        if (
                connection == active.connection and
                scan_datetime > active.departure
        ):
            active.record_soft_failure()

    def transform(self, paths, parent=None):

        if parent is None:
            parent = GraphNode(
                wbn=self.wbn, arrival=0, departure=0, state='Reached',
                destination=False
            )
            parent.save()

        active = None

        for index, path in enumerate(paths):
            state = 'future'
            is_destination = False

            if index == 0:
                state = 'active'

            if index == len(paths) - 1:
                is_destination = True

            element = GraphNode(
                wbn=self.wbn, arrival=path.arrival, departure=path.departure,
                state=state, destination=is_destination,
                vertex=DeliveryCenter.find_one({'code': path.origin}),
                parent=parent, edge=Connection.find_one(path.connection),
            )
            element.save()

            if active is None:
                active = element

            parent = element
            return active

    def handle_inscan(
            self, active, location, destination, scan_datetime, connection
    ):
        expected_at = GraphNode.find_by_parent(active)

        if location != expected_at.vertex.code:
            # Mark prior active node as failed(since it also carries the
            # forwarding connection to expected node). Mark the expected
            # node as failed and create a new route from this point onwards
            # specifying the parent as reached node
            active.deactivate()

            # Mark expected node as failed due to manual reroute to a different
            # center
            expected_at.fail_misroute()

            # Find new path from current location to destination
            path = self.marg.shortest_path(location, scan_datetime)[
                destination
            ]
            self.transform(path, active.parent)

        else:
            if connection != active.connection:
                # Reached the expected destination albeit via a different
                # connection than intended due to missing a connection or due
                # to manual override

                # Mark the expected node as failed due to failed
                # connection. If the reached time is less than
                # expected.outtime then just specify a new active as
                # reached and change parent for expected from active to
                # newactive, otherwise populate a new path and specify
                # parent as newactive

                # Mark last reached node as inactive(since it also carries
                # the forwarding connection to expected node).
                active.deactivate()

                # Duplicate active node specifying the used connection
                used_connection = Connection.find_one(connection)
                newactive = GraphNode(
                    wbn=active.wbn, vertex=active.vertex,
                    parent=active.parent, edge=used_connection,
                    departure=active.departure, arrival=scan_datetime)
                newactive.save()
                active = newactive

            active.reached()
            if scan_datetime > expected_at.departure:
                path = self.marg.shortest_path(
                    expected_at.vertex.code, scan_datetime
                )[destination]
                self.transform(path, active)
                expected_at.fail_delayed_arrival()
            else:
                expected_at.update_parent(active)
                expected_at.activate()

    def handle_destination(
            self, active, location, destination, scan_datetime
    ):
        try:
            # Validate that destination is unchanged
            GraphNode.find_one({
                'wbn': active.wbn,
                'vertex.code': destination,
                'destination': True
            })
        except ValueError:
            GraphNode.update(
                {'wbn': active.wbn, 'destination': True},
                {'$set': {'destination': False}}
            )
            active_parent = active.parent
            active.deactivate()
            path = self.marg.shortest_path(location, scan_datetime)[
                destination
            ]
            self.transform(path, active_parent)

    def update_path(
            self, location, destination, scan_datetime, action,
            connection
    ):
        try:
            active = GraphNode.find_one({'wbn': self.wbn, 'status': 'active'})
        except ValueError:
            path = self.marg.shortest_path(location, scan_datetime)[
                destination
            ]
            self.transform(path)

        if not action:
            self.handle_destination(
                active, location, destination, scan_datetime
            )
        elif action == '<L':
            self.handle_inscan(
                active, location, destination, scan_datetime, connection
            )
        elif action == '>L':
            self.handle_outscan(
                active, location, destination, scan_datetime, connection
            )
