from done.marg import Marg
from backend.models import (
    Connection, DeliveryCenter, GraphNode, CenterFailure,
    ConnectionFailure
)


class Graph:
    def __init__(self, wbn):
        self.wbn = wbn
        connections = [Connection(**data) for data in Connection.find({})]
        connections = [{
            'id': connection._id,
            'cutoff_departure': connection.departure.value,
            'duration': connection.duration.value,
            'origin': connection.origin.code,
            'destination': connection.destination.code
        } for connection in connections]
        self.marg = Marg(connections, json=True)

    def transform(self, paths, parent=None, sd=None):

        if parent is None:
            # Null element
            parent = GraphNode(
                wbn=self.wbn, state='reached'
            )
            parent.save()
            parent = parent._id

        active = None

        for index, path in enumerate(paths):

            if index > 0:
                state = 'future'
                e_arr = paths[index - 1]['arrival']
            else:
                state = 'active'
                e_arr = sd

            element = GraphNode(
                wbn=self.wbn, e_arr=e_arr,
                e_dep=path['departure'], state=state, destination=False,
                vertex=DeliveryCenter.find_one({'code': path['origin']}),
                parent=parent, edge=Connection.find_one(path['connection']),
            )
            element.save()

            if active is None:
                active = GraphNode.find_one(element._id)

            parent = element._id
        last_element = GraphNode(
            wbn=self.wbn, e_arr=paths[-1]['arrival'],
            state='future', destination=True, parent=parent,
            vertex=DeliveryCenter.find_one({
                'code': paths[-1]['destination']
            })
        )
        last_element.save()
        return active

    def handle_destination(
            self, active, location, destination, scan_datetime
    ):
        try:
            # Validate that destination is unchanged
            if GraphNode.find_one({
                    'wbn': active.wbn,
                    'destination': True
            }).vertex.code != destination:
                    raise ValueError('Mismatched destination')
        except ValueError:
            GraphNode.update(
                {'wbn': active.wbn, 'destination': True},
                {'$set': {'destination': False}}
            )
            active_parent = active.parent.value
            active.deactivate()
            path = self.marg.shortest_path(location, scan_datetime)[
                destination
            ]
            self.transform(path, active_parent)

    def handle_outscan(
            self, active, location, destination, scan_datetime, connection
    ):

        if connection == active.connection:
            if scan_datetime > active.e_dep:
                active.record_soft_failure(scan_datetime)
            else:
                active.a_dep = scan_datetime
                active.save()
        elif active.a_arr < active.e_dep:
            cf = CenterFailure(center=active.node, mroute=True)
            cf.save()
        else:
            cf = CenterFailure(center=active.node, cfail=True)
            cf.save()

    def handle_inscan(
            self, active, location, destination, scan_datetime, connection
    ):
        expected_at = GraphNode.find_by_parent(active)

        if location != expected_at.vertex.code:
            active.deactivate()

            path = self.marg.shortest_path(location, scan_datetime)[
                destination
            ]

            active = GraphNode(
                wbn=active.wbn, vertex=active.vertex.value,
                parent=active.parent.value,
                edge=Connection.find_one(connection), e_arr=active.e_arr,
                e_dep=active.e_arr, a_arr=active.a_arr,
                a_dep=active.a_dep, state='reached'
            )
            active.save()
            active = self.transform(path, active)
            active.a_arr = scan_datetime
            active.save()
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
                    parent=active.parent.value, edge=used_connection,
                    e_dep=active.departure, e_arr=scan_datetime,
                    a_dep=active.a_dep, a_arr=active.a_arr)
                newactive.save()
                active = newactive

            active.reached()
            expected_at.a_arr = scan_datetime
            expected_at.save()

            if scan_datetime > expected_at.departure:
                path = self.marg.shortest_path(
                    expected_at.vertex.code, scan_datetime
                )[destination]
                expected_at.deactivate()
                cf = ConnectionFailure(
                    connection=active.edge.value, fail_in=True, hard=True
                )
                cf.save()
                self.transform(path, active)
            else:
                expected_at.update_parent(active)
                expected_at.activate()

    def update_path(
            self, location, destination, scan_datetime, action,
            connection
    ):
        try:
            active = GraphNode.find_one({'wbn': self.wbn, 'state': 'active'})
        except ValueError:
            path = self.marg.shortest_path(location, scan_datetime)[
                destination
            ]
            active = self.transform(path)

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
