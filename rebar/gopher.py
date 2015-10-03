import datetime
from graphviz import Digraph

COLOR_MAP = {
    'active': 'darkgreen',
    'reached': 'chocolate4',
    'failed': 'crimson',
    'future': 'gold1',
    'inactive': 'gray87'
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
            dot.node(
                '{}'.format(node._id),
                label=vertex_code,
                color=COLOR_MAP[node.st]
            )
            vertex_map['{}'.format(node._id)] = node

    for node in nodes:
        if node.parent:
            parent = vertex_map['{}'.format(node.parent['_id'])]
            edge_name = 'NULL'

            if parent.edge:
                edge_name = parent.edge.get('name', 'NULL')

            dot.edge(
                '{}'.format(node.parent['_id']),
                '{}'.format(node._id),
                label=edge_name
            )

    name = '{}_{}'.format(
        waybill, datetime.datetime.now().strftime('%Y%m%dT%H%M%S')
    )
    name = 'graphs/{}'.format(name)
    dot.render(name, view=False)
