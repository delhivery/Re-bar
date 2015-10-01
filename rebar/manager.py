import datetime

from done.marg import Marg

from models.base import Connection, GraphNode


class GraphManager:
    def __init__(self, waybill):
        self.waybill = waybill
        connections = []

        for connection in Connection.all():
            connections.append({
                'id': connection._id,
                'cutoff_departure': connection.departure,
                'duration': connection.duration,
                'origin': connection.origin.code,
                'destination': connection.destination.code
            })
        self.marg = Marg(connections, json=True)

    def transform(self, paths, parent=None, scan_date=None):

        if parent is None:
            parent = GraphNode(wbn=self.waybill, state='reached')
            parent.save()

        active = None

        for index, path in enumerate(paths):
            a_arr = None

            if index > 0:
                state = 'future'
                e_arr = paths[index - 1]['arrival']
            else:
                state = 'active'
                e_arr = scan_date
                a_arr = scan_date
            graphnode = GraphNode(
                wbn=self.waybill, e_arr=e_arr, a_arr=a_arr,
                e_dep=path['departure'], state=state, parent=parent,
                vertex={'code': path['origin']},
                edge={'_id': path['connection']}
            )
            graphnode.save()

            if active is None:
                active = graphnode

            parent = graphnode

        destination_node = GraphNode(
            wbn=self.waybill, e_arr=paths[-1]['arrival'], state='future',
            dst=True, parent=parent, vertex={
                'code': paths[-1]['destination']
            }
        )
        destination_node.save()
        return active

    def handle_destination_scan(
        self, active, location, destination, scan_datetime
    ):
        node = GraphNode.find_one({'wbn': self.waybill, 'dst': True})

        if node.vertex.code != destination:
            print(
                'Destination Changed from {} to {}'.format(
                    node.vertex.code, destination
                )
            )
            path = self.marg.shortest_path(active.vertex.code, scan_datetime)[
                destination
            ]
            active.deactivate('dmod')
            return self.transform(path, active.parent)

    def handle_location_scan(
        self, active, location, destination, scan_datetime
    ):
        self.handle_destination_scan(
            active, location, destination, scan_datetime
        )

        if active.vertex.code != location:
            print('Vertex jump without IST from {} to {}'.format(
                active.vertex.code, location
            ))
            path = self.marg.shortest_path(location, scan_datetime)[
                destination
            ]
            active.deactivate('regen')

            active = GraphNode(
                vertex=active.vertex, e_arr=active.e_arr, a_arr=active.a_arr,
                e_dep=active.e_dep, state='reached'
            )
            active.save()
            return self.transform(path, active)

    def handle_outscan(
        self, active, location, destination, scan_datetime, connection
    ):
        if active.connection.idx != connection:
            if active.a_arr < active.e_dep:
                # Hard failure, center missed connection
                active.deactivate('hard', 'center')
            else:
                # Hard failure, connection arrived too late
                active.deactivate('hard', 'cin')

            connection = Connection.find_one({'idx': connection})
            e_arr = active.e_arr
            e_dep = e_arr.replace(
                hour=connection.departure.hour,
                minute=connection.departure.minute,
                second=connection.departure.second
            )

            if e_dep < e_arr:
                e_dep = e_dep + datetime.timedelta(days=1)

            active = GraphNode(
                vertex=active.vertex, e_arr=active.e_arr, a_arr=active.a_arr,
                e_dep=e_dep, a_dep=scan_datetime, edge=connection,
                state='reached'
            )
            active.save()

            e_arr = e_dep + datetime.timedelta(seconds=connection.duration)
            path = self.marg.shortest_path(connection.destination.code, e_arr)[
                destination
            ]
            return self.transform(path, parent=active)
        else:
            active.a_dep = scan_datetime
            if active.a_dep > active.e_dep:
                active.reached('soft', 'cout')
            else:
                active.reached()
            active = GraphNode.find_by_parent(active)
            active.activate()
            return active

    def handle_inscan(
        self, active, location, destination, scan_datetime, connection
    ):
        active.a_arr = scan_datetime

        if active.vertex.code != location:
            active.deactivate('mroute', 'center')
            path = self.marg.shortest_path(location, scan_datetime)[
                destination
            ]
            return self.transform(path.parent, scan_datetime)

        if active.e_dep < active.a_arr:
            path = self.marg.shortest_path(active.vertex.code, active.a_arr)[
                destination
            ]
            active.deactivate('hard', 'cin')
            return self.transform(path, parent=active.parent)
        elif active.a_arr > active.e_arr:
            active.reached('soft', 'cin')
        else:
            active.reached()
        active = GraphNode.find_by_parent(active)
        active.activate()
        return active

    def parse_path(
        self, location, destination, scan_datetime, action, connection
    ):
        if connection:
            try:
                connection = int(connection)
            except ValueError:
                pass

            if isinstance(connection, int):
                try:
                    connection = Connection.find_one({'index': connection})
                except ValueError:
                    print('Ignoring connection {}'.format(connection))
                    raise
        try:
            is_complete = GraphNode.find(
                {'wbn': self.waybill, 'state': 'reached', 'destination': True},
                count=True
            )

            if is_complete > 0:
                print('Waybill {} has already reached destination'.format(
                    self.waybill
                ))
                return

            active = GraphNode.find_one(
                {'wbn': self.waybill, 'state': 'active'}
            )
        except ValueError:
            path = self.marg.shortest_path(location, scan_datetime)[
                destination
            ]
            active = self.transform(path, sd=scan_datetime)

        if not action:
            self.handle_location_scan(
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