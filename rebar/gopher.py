import datetime
from graphviz import Digraph


def plot_graph(nodes, name=None):
    waybill = None

    if nodes:
        waybill = nodes[0].wbn

    dot = Digraph(comment='{}'.format(waybill))

    for node in nodes:
        vertex_code = 'NULL'
        edge_name = 'NULL'
        edge_origin = 'NULL'
        edge_destin = 'NULL'
        edge_depart = 'NULL'

        if node.vertex:
            vertex_code = node.vertex.code

        if node.edge:
            edge_name = node.edge.name
            edge_origin = node.edge.origin.code
            edge_destin = node.edge.destination.code
            edge_depart = node.edge.departure

        dot.node(
            vertex_code, edge_name,
            label='Connection from {} to {} at {}'.format(
                edge_origin, edge_destin, edge_depart
            )
        )

    if not name:
        name = '{}_{}'.format(
            waybill, datetime.datetime.now().strftime('%Y%m%dT%H%M%S')
        )
    name = 'graphs/{}'.format(name)
    dot.render(name, view=False)
