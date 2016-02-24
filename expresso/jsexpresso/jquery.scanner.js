(function ($) {
  $.fn.scanner = function (data) {
    $.fn.scanner.scans = []
    $.fn.scanner.segments = []

    $(document).on('click', '.element', function () {
      $('.element').removeClass('selected')
      $(this).addClass('selected')
      $('#graph_container').grapher($.fn.scanner.segments[$(this).data('idx')])
    })

    function drawContainers () {
      var graphCanvas = $('<div />')
                   .addClass('graph')
                   .append($('<div />')
                           .addClass('graph_container')
                           .attr('id', 'graph_container'))
      var scanCanvas = $('<div />')
                       .addClass('scans')
                       .append(
                            $('<div />')
                            .addClass('scan_list'))
      var canvases = []
      canvases.push(graphCanvas)
      canvases.push(scanCanvas)
      return canvases
    }

    function clear (obj) {
      $(obj).html('')
    }

    clear(this)
    this.append(drawContainers())

    $.each(data, function (idx) {
      var record = data[idx]
      var scan = record.scan
      var scan_segments = record.segments
      $.fn.scanner.scans.push(scan)
      $.fn.scanner.segments.push(scan_segments)
    })

    var scan_blocks = []

    $.each($.fn.scanner.scans, function (idx) {
      scan_blocks.push($.fn.scanner.makeScan(idx, $.fn.scanner.scans[idx]))
    })

    this.find('.scan_list').append(scan_blocks)
    this.find('.scan_list').append($.fn.scanner.makeArrow())

    this.find('.scan_list .element:first').click()
  }

  $.fn.scanner.makeScan = function (idx, scan) {
    var scan_block = $('<div />')
                     .addClass('scan')
                     .addClass('element')
                     .addClass('has_tooltip')
                     .text(scan.act)
                     .data('idx', idx)
                     .append($.fn.scanner.makeTooltip(scan))
    return scan_block
  }

  $.fn.scanner.makeTooltip = function (scan) {
    function getISODateString (value) {
      if (isNaN(parseInt(value))) {
        return ''
      }
      var date = new Date(parseInt(value) * 1000)
      return date.toISOString()
    }

    var tooltipBlock = $('<ul />')
                       .addClass('tipster')
                       .append($.fn.grapher.makeTooltipElement('Date', getISODateString(scan.sdt)))
                       .append($.fn.grapher.makeTooltipElement('Location', scan.src))
                       .append($.fn.grapher.makeTooltipElement('Connection', scan.cid))
                       .append($.fn.grapher.makeTooltipElement('Promise Date', getISODateString(scan.pdd)))
    return tooltipBlock
  }

  $.fn.scanner.makeArrow = function () {
    var arrowBlock = $('<div />')
                     .addClass('scan')
                     .addClass('arrow')
    return arrowBlock
  }
}(jQuery))
