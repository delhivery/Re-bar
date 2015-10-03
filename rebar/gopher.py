import datetime
from graphviz import Digraph

COLOR_MAP = {
    'active': '#00A0B0',
    'reached': '#6A4A3C',
    'failed': '#CC333F',
    'future': '#EDC951',
    'inactive': '#E0E4CC'
}


def plot_graph(nodes):
    waybill = None

    if nodes:
        waybill = nodes[0].wbn

    dot = Digraph(comment='{}'.format(waybill), format='png')

    vertex_map = {}

    for node in nodes:
        vertex_code = 'NULL'

        if node.vertex:
            vertex_code = node.vertex.code

        if '{}'.format(node._id) not in vertex_map:
            vertex_map['{}'.format(node._id)] = (
                dot.node('{}'.format(
                    node._id, color=COLOR_MAP[node.st]),
                    label=vertex_code
                ),
                node
            )

    for node in nodes:
        if node.parent:
            parent_node, parent = vertex_map['{}'.format(node.parent['_id'])]

            connection_name = parent.connection.get('name', 'NULL')

            dot.edge(
                '{}'.format(node.parent['_id']),
                '{}'.format(node._id),
                label=connection_name
            )

    name = '{}_{}'.format(
        waybill, datetime.datetime.now().strftime('%Y%m%dT%H%M%S')
    )
    name = 'graphs/{}'.format(name)
    dot.render(name, view=False)
