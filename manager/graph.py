from done.marg import Marg
from backend.models import (
    Connection, DeliveryCenter, GraphNode, CenterFailure,
    ConnectionFailure
)

from mongo.utils import serialize


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
            a_arr = None

            if index > 0:
                state = 'future'
                e_arr = paths[index - 1]['arrival']
            else:
                state = 'active'
                e_arr = sd
                a_arr = sd

            element = GraphNode(
                wbn=self.wbn, e_arr=e_arr, a_arr=a_arr,
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
            }).vertex.value.code != destination:
                print('Destination change detected')
                raise ValueError('Mismatched destination')
        except ValueError:
            import ipdb
            ipdb.set_trace()
            print('Destination change detected')
            GraphNode.update(
                {'wbn': active.wbn, 'destination': True},
                {'$set': {'destination': False}}
            )
            active_parent = active.parent.value
            active.deactivate()
            path = self.marg.shortest_path(location, scan_datetime)[
                destination
            ]
            print('Creating a new path from {} to {} at {}'.format(
                location, destination, scan_datetime
            ))
            self.transform(path, active_parent)

    def handle_outscan(
            self, active, location, destination, scan_datetime, connection
    ):

        if connection == active.connection:
            print('Connection taken same as active')
            if scan_datetime > active.e_dep.value:
                print('Connected post expected departure')
                active.record_soft_failure(scan_datetime)
            else:
                print('Connected in time')
                active.a_dep.valueOf(scan_datetime)
                active.save()
        elif active.a_arr.value < active.e_dep.value:
            print('Connection taken not same as recommended but shipment arrived in time for outbound')
            cf = CenterFailure(center=active.vertex.value, mroute=True)
            cf.save()
        else:
            print('Connection taken not same as recommended but shipment did not arrive in time for outbound')
            cf = CenterFailure(center=active.vertex.value, cfail=True)
            cf.save()

    def handle_inscan(
            self, active, location, destination, scan_datetime, connection
    ):
        expected_at = GraphNode.find_by_parent(active)
        print('Expected at: {}. Reached {}'.format(expected_at.vertex.value.code, location))
        print('Expected via: {} Reached via {}'.format(active.edge.value._id, connection))

        if location != expected_at.vertex.value.code:
            print('Location mismatch. Expected {} vs {}'.format(expected_at.vertex.value.code, location))
            print('Deactivating last active: {}'.format(active.vertex.value.code))
            active.deactivate()

            path = self.marg.shortest_path(location, scan_datetime)[
                destination
            ]
            print('Generated new path from {} to {} at {}'.format(location, destination, scan_datetime))

            active = GraphNode(
                wbn=active.wbn, vertex=active.vertex.value,
                parent=active.parent.value,
                edge=Connection.find_one(connection), e_arr=active.e_arr.value,
                e_dep=active.e_arr.value, a_arr=active.a_arr.value,
                a_dep=active.a_dep.value, state='reached'
            )
            print('Duplicated active for connection {}'.format(connection))
            active.save()
            active = self.transform(path, active)
            active.a_arr.valueOf(scan_datetime)
            active.save()
        else:
            if connection != '{}'.format(active.edge.value._id):
                print('Mismatched connection expected {} vs {}'.format(active.edge.value._id, connection))
                print('Deactivating last active: {}'.format(active.vertex.value.code))
                active.state = 'failed'
                active.save()

                # Duplicate active node specifying the used connection
                used_connection = Connection.find_one(connection)
                newactive = GraphNode(
                    wbn=active.wbn, vertex=active.vertex.value,
                    parent=active.parent.value, edge=used_connection,
                    e_dep=active.e_dep.value, e_arr=scan_datetime,
                    a_dep=active.a_dep.value, a_arr=active.a_arr.value)
                newactive.save()
                active = newactive

            print('Marking active node as reached')
            active.reached()
            print('Marking arrival time at expected node')
            expected_at.a_arr.valueOf(scan_datetime)
            expected_at.save()

            if expected_at.destination:
                expected_at.state = 'reached'
                expected_at.update_parent(active)
                print('Traversal Complete')
            elif scan_datetime > expected_at.e_dep.value:
                print('Connection arrived too late to depart in time')
                path = self.marg.shortest_path(
                    expected_at.vertex.value.code, scan_datetime
                )[destination]
                print('Populating new path from {} to {} at {}'.format(expected_at.vertex.value.code, destination, scan_datetime))
                expected_at.deactivate()
                print('Deactivating old expected. Recording connection failure for connection to expected node')
                cf = ConnectionFailure(
                    connection=active.edge.value, fail_in=True, hard=True
                )
                cf.save()
                self.transform(path, parent=active, sd=scan_datetime)
                print('Saved new path')
            else:
                print('Changed parent for expected node {} to {} and marked it as active'.format(expected_at.vertex.value.code, active.vertex.value.code))
                expected_at.state = 'active'
                expected_at.update_parent(active)
                print('New active is: {}'.format(serialize(expected_at)))

    def update_path(
            self, location, destination, scan_datetime, action,
            connection
    ):
        print()
        print()
        print('Scanning for status')
        try:
            is_complete = GraphNode.find({'wbn': self.wbn, 'state': 'reached', 'destination': True})

            if is_complete.count() > 0:
                print('Traversal for this waybill is already complete')
                return

            active = GraphNode.find_one({'wbn': self.wbn, 'state': 'active'})
            print('Found an active node')
        except ValueError:
            print(
                'No graph found. Created a new one from {} to {} at {}'.format(
                    location, destination, scan_datetime
                )
            )
            path = self.marg.shortest_path(location, scan_datetime)[
                destination
            ]
            active = self.transform(path, sd=scan_datetime)
        print('Current active node: {}'.format(active.vertex.value.code))
        if not action:
            self.handle_destination(
                active, location, destination, scan_datetime
            )
        elif action == '<L':
            print('Inbound detected.')
            self.handle_inscan(
                active, location, destination, scan_datetime, connection
            )
        elif action == '+L':
            print('Outbound detected.')
            self.handle_outscan(
                active, location, destination, scan_datetime, connection
            )
