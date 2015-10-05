import datetime

from graphviz import Digraph
from models.base import DeliveryCenter


COLOR_MAP = {
    'active': 'darkgreen',
    'reached': 'chocolate4',
    'failed': 'crimson',
    'future': 'gold1',
    'inactive': 'gray87'
}


def plot_graph(waybill, nodes):

    dot = Digraph(comment='{}'.format(waybill), format='png')

    vertex_map = {}

    for node in nodes:
        vertex_code = 'NULL'
        node_id = '{}'.format(node._id)

        if node.vertex:
            vertex_code = node.vertex.name

        label = '{} EArr: {} EDep: {} Arr: {} Dep: {}'.format(
            vertex_code, node.e_arr, node.e_dep, node.a_arr, node.a_dep
        )
        dot.node(
            node_id, label=label, color=COLOR_MAP[node.st]
        )
        vertex_map[node_id] = node

    for node in nodes:
        if node.parent:
            node_id = '{}'.format(node._id)
            parent_id = '{}'.format(node.parent['_id'])

            parent = vertex_map[parent_id]
            edge_name = 'NULL'

            if parent.edge:
                edge_name = parent.edge.get('name', 'NULL')

            dot.edge(parent_id, node_id, label=edge_name)

    name = '{}_{}'.format(
        waybill, datetime.datetime.now().strftime('%Y%m%dT%H%M%S')
    )
    name = 'graphs/{}'.format(name)
    dot.render(name, view=False)
