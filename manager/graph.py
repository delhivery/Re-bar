from done.marg import Marg
from backend.models import Connection, GraphNode


class Graph:
    def __init__(self, wbn):
        self.wbn = wbn
        self.marg = Marg(Connection.all())

    def get_active(self):
        return GraphNode.find({'wbn': self.wbn, 'state': 'active'})

    def next(self, node):
        return GraphNode.find({'wbn': self.wbn, 'parent': node['_id']})

    def activate_next(self, node):
        next_node = self.next(node)
        node.mark_reached()
        next_node.mark_active()

    def record_delayed_connectivity(self, connection):
        pass

    def parse_scan(
            self, action, connection, location, destination, scan_date
    ):
        active = self.get_active()
        connection_recommended = active.get_connection()
        connection_recommended_str = '{}'.format(
            connection_recommended['_id']
        )

        if action == 'Connect':
            # Added to next connection
            # Record Soft Error if connection was connected late
            if connection == connection_recommended_str:
                if scan_date > active['departure']:
                    self.record_delayed_connectivity(connection_recommended)

        if action == 'Recieve':
            # Reached Next Node

            if connection != connection_recommended_str:
                # Arrival via incorrect connection

                if active['destination'] != destination:
                    # Arrival at incorrect destination
                    self.record_missed_destination(active['destination'])

                    # TODO
                    # get new path and append
                else:
                    self.record_missed_connection(connection_recommended)

                    # Mark active as future

                    # TODO
                    # If future can not be met get new path and append to
                    # parent else duplicate future path. Mark new active
                    # node and reached node
            else:
                pass

    def parser(
            self, waybill, location, destination, scan_datetime, action,
            connection):

        if GraphNode.find_one({'wbn': waybill}) is None:
            if action in [None, 'inscan']:
                newpath = self.marg.shortest_path(location, scan_datetime)[
                    destination
                ]
                # Convert path to GraphNode and save it to database
                # Also set first node to active, root node to root_node and
                # last node to Destination
                newpath.transform()
        self.update_path(
            waybill, location, destination, scan_datetime, action, connection
        )

    def handle_outscan(
            self, active, location, destination, scan_datetime, connection
    ):

        if (
                connection == active.connection and
                scan_datetime > active.departure
        ):
            active.record_soft_failure()

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
            newpath = self.marg.shortest_path(location, scan_datetime)[
                destination
            ]
            newpath.transform(active.parent)

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
                newpath = self.marg.shortest_path(
                    expected_at.vertex.code, scan_datetime
                )[destination]
                newpath.transform(active)
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
            newpath = self.marg.shortest_path(location, scan_datetime)[
                destination
            ]
            newpath.transform(active_parent)

    def update_path(
            self, waybill, location, destination, scan_datetime, action,
            connection
    ):
        active = GraphNode.find_one({'wbn': waybill, 'status': 'active'})

        if not action:
            self.handle_destination(
                active, location, destination, scan_datetime
            )
        elif action == 'inscan':
            self.handle_inscan(
                active, location, destination, scan_datetime, connection
            )
        elif action == 'Outscan':
            self.handle_outscan(
                active, location, destination, scan_datetime, connection
            )
