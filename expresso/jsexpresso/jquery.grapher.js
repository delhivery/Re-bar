(function ($) {
  $.fn.grapher = function (data) {
    function makeNodes (data) {
      $.each(data, function (record_idx) {
        var record = data[record_idx]
        var node = {
          idx: record.idx,
          lab: record.src,
          pdt: record.p_dep,
          pat: record.p_arr,
          adt: record.a_dep,
          aat: record.a_arr,
          rmk: record.rmk,
          par: record.par,
          sta: record.st,
          cst: record.cst,
          sol: record.sol,
          sub: 0
        }

        if (node.par !== null && node.par !== undefined) {
          $.fn.grapher.nodes[node.par].sub++
          node.posx = $.fn.grapher.nodes[node.par].posx + 2 * $.fn.grapher.defaults.NODE_BREAK_X
          node.posy = $.fn.grapher.nodes[node.par].posy + ($.fn.grapher.nodes[node.par].sub - 1) * 2 * $.fn.grapher.defaults.NODE_BREAK_Y
        } else {
          node.posx = $.fn.grapher.defaults.NODE_START_X
          node.posy = $.fn.grapher.defaults.NODE_START_Y
        }
        $.fn.grapher.nodes.push(node)
      })
    }

    function makeEdges (data) {
      $.each(data, function (record_idx) {
        var record = data[record_idx]

        if (record.par !== null && record.par !== undefined) {
          var edge = {
            lab: data[record.par].conn,
            src: record.par,
            dst: record.idx,
            sta: record.st
          }

          if ($.fn.grapher.nodes[edge.src].adt !== null && $.fn.grapher.nodes[edge.src] !== undefined) {
            if ($.fn.grapher.nodes[edge.dst].aat !== null && $.fn.grapher.nodes[edge.dst].aat !== undefined) {
              edge.sta = $.fn.grapher.nodes[edge.src].sta
            }
          }

          edge.posx = $.fn.grapher.nodes[edge.src].posx + $.fn.grapher.defaults.NODE_BREAK_X
          edge.posy = $.fn.grapher.nodes[edge.src].posy + $.fn.grapher.defaults.NODE_BREAK_Y / 2
          var dst_y = $.fn.grapher.nodes[edge.dst].posy - $.fn.grapher.nodes[edge.src].posy
          edge.length = Math.sqrt($.fn.grapher.defaults.NODE_BREAK_X * $.fn.grapher.defaults.NODE_BREAK_X + dst_y * dst_y)
          edge.rotation = Math.atan(dst_y / $.fn.grapher.defaults.NODE_BREAK_X)
          $.fn.grapher.edges.push(edge)
        }
      })
    }

    function clear (obj) {
      $(obj).html('')
    }

    $.fn.grapher.nodes = []
    $.fn.grapher.edges = []
    makeNodes(data)
    makeEdges(data)
    clear(this)
    var blocks = []
    blocks.push.apply(blocks, $.fn.grapher.plotNodes())
    blocks.push.apply(blocks, $.fn.grapher.plotEdges())
    this.append(blocks)
  }

  $.fn.grapher.defaults = {
    NODE_UNITS: 'em',
    NODE_BREAK_X: 15,
    NODE_BREAK_Y: 3,
    NODE_START_X: 0,
    NODE_START_Y: 0
  }

  $.fn.grapher.nodes = []

  $.fn.grapher.edges = []

  $(document).on('mouseenter', '.has_tooltip:not(.tooltipstered)', function () {
    $(this).tooltipster({
      functionInit: function (origin, content) {
        return origin.find('.tipster')
      },
      theme: 'tooltipster-shadow'
    }).tooltipster('show')
  })

  $.fn.grapher.plotEdges = function () {
    var edge_blocks = []
    $.each($.fn.grapher.edges, function (edge_idx) {
      edge_blocks.push($.fn.grapher.plotEdge($.fn.grapher.edges[edge_idx]))
    })
    return edge_blocks
  }

  $.fn.grapher.plotEdge = function (edge) {
    var edge_block = $('<div />')
                     .addClass('edge')
                     .addClass(edge.sta)
                     .html(edge.lab)
                     .css('top', edge.posy + $.fn.grapher.defaults.NODE_UNITS)
                     .css('left', edge.posx + $.fn.grapher.defaults.NODE_UNITS)
                     .css('width', edge.length + $.fn.grapher.defaults.NODE_UNITS)
                     .css('transform', 'rotate(' + edge.rotation + 'rad)')
    return edge_block
  }

  $.fn.grapher.plotNodes = function () {
    var node_blocks = []
    $.each($.fn.grapher.nodes, function (node_idx) {
      node_blocks.push($.fn.grapher.plotNode($.fn.grapher.nodes[node_idx]))
    })
    return node_blocks
  }

  $.fn.grapher.plotNode = function (node) {
    var node_block = $('<div />')
                     .addClass('vertex')
                     .addClass('has_tooltip')
                     .addClass(node.sta)
                     .addClass(node.sol)
                     .html(node.lab)
                     .css('top', node.posy + $.fn.grapher.defaults.NODE_UNITS)
                     .css('left', node.posx + $.fn.grapher.defaults.NODE_UNITS)
                     .append($.fn.grapher.makeTooltip(node))
    return node_block
  }

  $.fn.grapher.makeTooltip = function (node) {
    function getISODateString (value) {
      if (isNaN(parseInt(value))) {
        return ''
      }
      var date = new Date(parseInt(value) * 1000)
      return date.toISOString()
    }

    var tooltip = $('<ul />')
                  .addClass('tipster')
                  .append($.fn.grapher.makeTooltipElement('Predicted Departure', getISODateString(node.pdt)))
                  .append($.fn.grapher.makeTooltipElement('Predicted Arrival', getISODateString(node.pat)))
                  .append($.fn.grapher.makeTooltipElement('Actual Departure', getISODateString(node.adt)))
                  .append($.fn.grapher.makeTooltipElement('Actual Arrival', getISODateString(node.aat)))
                  .append($.fn.grapher.makeTooltipElement('Cumulative Cost', node.cst))
                  .append($.fn.grapher.makeTooltipElement('Remarks', node.rmk))
    return tooltip
  }

  $.fn.grapher.makeTooltipElement = function (label, value) {
    var tooltipElement = $('<li />')
                         .addClass('data')
                         .append($.fn.grapher.makeTooltipValueBlock('key', label))
                         .append($.fn.grapher.makeTooltipValueBlock('value', value))
    return tooltipElement
  }

  $.fn.grapher.makeTooltipValueBlock = function (className, value) {
    var tooltipValueBlock = $('<span />')
                       .addClass(className)
                       .html(value)
    return tooltipValueBlock
  }
}(jQuery))
