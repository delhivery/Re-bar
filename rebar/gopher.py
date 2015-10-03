import datetime
from graphviz import Digraph


def plot_graph(nodes, name=None):
    waybill = None

    if nodes:
        waybill = nodes[0].wbn

    dot = Digraph(comment='{}'.format(waybill))

    vertex_map = {}

    for node in nodes:
        vertex_code = 'NULL'

        if node.vertex:
            vertex_code = node.vertex.code

        if node._id not in vertex_map:
            vertex_map[node._id] = (
                dot.node(node._id, label=vertex_code), node
            )

    for node in nodes:
        if node.parent:
            parent_node, parent = vertex_map[node.parent._id]
            dot.edge(node.parent._id, node._id, label=parent.connection.name)

    if not name:
        name = '{}_{}'.format(
            waybill, datetime.datetime.now().strftime('%Y%m%dT%H%M%S')
        )
    name = 'graphs/{}'.format(name)
    dot.render(name, view=False)
