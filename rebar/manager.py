'''
This module exposes the manager for the graph representing the EP
lifecycle of a package
'''
import logging
import datetime

from ..models.base import Connection, GraphNode

logging.basicConfig(filename='progress.log', level=logging.DEBUG)


class GraphManager:
    '''
    A manager class for a waybill which is responsible for parsing a scan
    against it and then either generating or updating the graph as needed
    '''

    def __init__(self, waybill, marg):
        self.waybill = waybill
        self.marg = marg

    def transform(self, paths, parent=None, scan_date=None, pickup_date=None):
        '''
        Transform a path from done.Marg.shortest_path to list of GraphNodes
        to be stored against the database
        '''

        if parent is None:
            parent = GraphNode(wbn=self.waybill, st='reached', pd=pickup_date)
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
                e_dep=path['departure'], st=state, parent=parent,
                p_con=parent.edge,
                vertex={'code': path['origin']},
                edge={'_id': path['connection']},
                pd=pickup_date
            )
            graphnode.save()

            if active is None:
                active = graphnode

            parent = graphnode

        destination_node = GraphNode(
            wbn=self.waybill, e_arr=paths[-1]['arrival'], st='future',
            dst=True, parent=parent, p_con=parent.edge, vertex={
                'code': paths[-1]['destination']
            }, pd=pickup_date
        )
        destination_node.save()
        return active

    def handle_destination_scan(
            self, active, destination, scan_datetime, pickup_date=None):
        '''
        Handle a change in destination of the waybill and regenerates the graph
        '''
        node = GraphNode.find_one({'wbn': self.waybill, 'dst': True})

        if node.vertex.code != destination:
            path = self.marg.shortest_path(
                active.vertex.code, scan_datetime
            )[destination]
            active.deactivate('dmod')
            return self.transform(path, active.parent, pickup_date=pickup_date)

    def handle_location_scan(
            self, active, location, destination, scan_datetime, **kwargs):
        '''
        Handle a location scan on a waybill and update the corresponding graph
        Also verifies if the destination of the waybill has changed
        '''
        pickup_date = kwargs.get('pd', None)
        self.handle_destination_scan(
            active, destination, scan_datetime, pickup_date)

        if active.vertex.code != location:
            path = self.marg.shortest_path(
                location, scan_datetime
            )[destination]
            active.deactivate('regen')

            active = GraphNode(
                wbn=self.waybill, vertex=active.vertex, e_arr=active.e_arr,
                a_arr=active.a_arr, e_dep=active.e_dep, st='reached',
                pd=pickup_date)
            active.save()
            return self.transform(path, active, pickup_date=pickup_date)

    def handle_outscan(
            self, active, destination, scan_datetime, connection,
            **kwargs):
        '''
        Handle an outscan on a waybill and update the corresponding graph
        '''
        pickup_date = kwargs.get('pd', None)

        if active.edge.index != connection.index:
            if active.a_arr < active.e_dep:
                # Hard failure, center missed connection
                active.deactivate('hard', 'center')
            else:
                # Hard failure, connection arrived too late
                active.deactivate('hard', 'cin')
            e_arr = active.e_arr
            e_dep = e_arr.replace(
                hour=connection.departure.hour,
                minute=connection.departure.minute,
                second=connection.departure.second
            )

            if e_dep < e_arr:
                e_dep = e_dep + datetime.timedelta(days=1)

            active = GraphNode(
                wbn=self.waybill, vertex=active.vertex, e_arr=active.e_arr,
                a_arr=active.a_arr, e_dep=e_dep, a_dep=scan_datetime,
                edge=connection, parent=active.parent, st='reached',
                pd=pickup_date)
            active.save()

            e_arr = e_dep + datetime.timedelta(seconds=connection.duration)

            if connection.destination.code != destination:
                path = self.marg.shortest_path(
                    connection.destination.code, e_arr).get(
                        destination, [])
                active = self.transform(
                    path, parent=active, pickup_date=pickup_date)
            else:
                active = GraphNode(
                    wbn=self.waybill, vertex=connection.destination,
                    e_arr=e_arr, st='active', parent=active, dst=True,
                    pd=pickup_date)
                active.save()

            active.e_arr = e_arr
            active.save()
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
            self, active, location, destination, scan_datetime, **kwargs):
        '''
        Handle an inscan on a waybill and update the corresponding graph
        '''
        pickup_date = kwargs.get('pd', None)
        active.a_arr = scan_datetime

        if active.vertex.code != location:
            active.deactivate('mroute', 'center')
            path = self.marg.shortest_path(location, scan_datetime)[
                destination
            ]
            return self.transform(
                path, active.parent, scan_datetime, pickup_date=pickup_date)

        if active.e_dep is None and active.dst:
            if active.a_arr > active.e_arr:
                active.reached('soft', 'cin')
            else:
                active.reached()
        elif active.e_dep < active.a_arr:
            path = self.marg.shortest_path(
                active.vertex.code, active.a_arr
            )[destination]
            active.deactivate('hard', 'cin')
            active = self.transform(
                path, parent=active.parent, scan_date=active.e_arr,
                pickup_date=pickup_date)
            active.a_arr = scan_datetime
            active.save()
        elif active.a_arr > active.e_arr:
            active['stc'] = 'soft'
            active['f_at'] = 'cin'
            active.save()
        else:
            active.save()
        return active

    def parse_path(self, **kwargs):
        '''
        Parse a scan against a waybill to populate/update its graph
        '''
        logging.debug('\n\nKwargs received: {}'.format(kwargs))

        location = kwargs.get('location', None)
        destination = kwargs.get('destination', None)
        scan_datetime = kwargs.get('scan_datetime', None)
        action = kwargs.get('action', None)
        connection = kwargs.get('connection', None)
        pickup_date = kwargs.get('pickup_date', None)
        try:
            connection = int(connection)
            connection = Connection.find_one({'index': connection})

            if not connection:
                raise ValueError()
        except (TypeError, ValueError):
            pass

        is_complete = GraphNode.count(
            {'wbn': self.waybill, 'st': 'reached', 'dst': True}
        )

        if is_complete > 0:
            return

        active = GraphNode.find_one(
            {'wbn': self.waybill, 'st': 'active'}
        )

        if active is None:
            # We can't parse a path at its destination

            if location == destination:
                return

            path = self.marg.shortest_path(
                location, scan_datetime
            )[destination]
            active = self.transform(
                path, scan_date=scan_datetime, pickup_date=pickup_date)

        if active.vertex.code != location or not action:
            self.handle_location_scan(
                active, location, destination, scan_datetime, pd=pickup_date)

        if action == '<L':
            self.handle_inscan(
                active, location, destination, scan_datetime, pd=pickup_date)
        elif action == '+L':
            self.handle_outscan(
                active, destination, scan_datetime, connection,
                pd=pickup_date
            )
