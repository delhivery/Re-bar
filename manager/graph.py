# from done.marg import Marg
from backend.models import GraphNode


class Graph:
    def __init__(self, wbn):
        self.wbn = wbn

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
        has_path = False

        if GraphNode.find_one({'wbn': waybill}) is None:
            if action in [None, 'inscan']:
                root_node = empty_root(waybill)
                newpath = Marg.shortest_path(location, scan_datetime)[destination]
                # Convert path to GraphNode and save it to database
                # Also set first node to active, root node to root_node and
                # last node to Destination
                newpath.transform(root_node)
                has_path = True
        else:
            has_path = True
        self.update_path(
            waybill, location, destination, scan_datetime, action, connection
        )

    def update_path(
            self, waybill, location, destination, scan_datetime, action,
            connection):

        if not action:
            current_destination = GraphNode.find_one({'wbn': waybill, 'destination': True})

            if current_destination != destination:
                GraphNode.update(
                    {'wbn': waybill, 'destination': True},
                    {'$set': {'destination': False}}
                )
                active = GraphNode.find_one({'wbn': waybill, 'status': 'active'})
                active_parent = active.parent
                avtice.deactivate()
                newpath = Marg.shortest_path(location, scan_datetime)[destination]
                newpath.transform(active_parent)
        elif action == 'inscan':
            active = GraphNode.find_one({'wbn': waybill, 'state': 'active'})
            expected = GraphNode.find_by_parent(active)

            if location != expected['vertex']['code']:
                active.reached()
                expected.fail_misroute()
                newpath = Marg.shortest_path(location, scan_datetime)[destination]
                newpath.transform(active)
            elif connection != active.connection:
                active.deactivate()
                newactive = GraphNode(wbn=waybill, vertex=active.vertex, parent=active.parent, edge=Connection.find_one({'_id': connection}), arrival=scan_datetime, departure=
