var fs = require('fs');
var net = require('net');
var Utils = require('./utils')


VERTEX_NAME_CODE_MAPPING = require('./mapper.json')


var Class = function(methods) {
    var klass = function() {
        this.initialize.apply(this, arguments);
    };

    for (var property in methods) {
       klass.prototype[property] = methods[property];
    }

    if (!klass.prototype.initialize) klass.prototype.initialize = function(){};

    return klass;
};


var Parser = Class({
    initialize: function(){
        this.active = null
        this.__segments = []
    },

    deactivate: function(){
        that = this
        that.__segments.forEach(function(val, idx){
            if (val['st'] === 'FUTURE'){
                that.__segments[idx]['st'] = 'INACT'
            }
        })

    },

    arrival: function(sdt){
        if (sdt === undefined){
            return this.__segments[this.active]['a_arr']
        } else {
            this.__segments[this.active]['a_arr'] = sdt
        }
    },

    add_nullseg: function(){
        var segment = {
            'src': null,
            'dst': null,
            'conn': null,
            'p_arr': null,
            'a_arr': null,
            'p_dep': null,
            'a_dep': null,
            'cst': null,
            'st': 'REACHED',
            'rmk': [],
            'idx': 0,
            'par': null,
            'sol': null,
            'pdd': null,
        }
        this.__segments.push(segment)
        this.active = 0
    },

    add_segment: function(subgraph, kwargs){

        if (subgraph === true){
            if (this.__segments.length === 0){
                this.add_nullseg()
            }
        }
        var segment = {
            'src': kwargs['src'],
            'dst': kwargs['dst'],
            'conn': kwargs['conn'],
            'p_arr': kwargs['p_arr'],
            'p_dep': kwargs['p_dep'],
            'cst': kwargs['cst'],
            'a_arr': kwargs['a_arr'] ||  null,
            'a_dep': kwargs['a_dep'] || null,
            'st':  kwargs['st'] || 'FUTURE',
            'rmk': kwargs['rmk'] || [],
        }

        for (var key in kwargs){

            if (['src', 'dst', 'conn', 'p_arr', 'p_dep', 'cst', 'a_arr', 'a_dep', 'st', 'rmk', 'idx', 'par'].indexOf(key) < 0){
                segment[key] = kwargs[key]
            }
        }

        segment['idx'] = kwargs['idx'] || this.__segments.length

        if (subgraph != true){
            var par = kwargs['par']
            if (par === undefined){
                par = this.__segments.length - 1
            }
            segment['par'] = par
        } else{
            segment['par'] = this.active
            segment['st'] = 'ACTIVE'
        }

        if (segment['st'] === 'ACTIVE') {
            this.active = segment['idx']
        }

        this.__segments.push(segment)
        return this.__segments.length
    },

    add_segments: function(segments, novi){
        var self = this
        if (novi === undefined){
            novi = false
        }
        segments.forEach(function(segment){
            try{
                self.add_segment(novi, segment)
            } catch(err){
                console.error('ERROR: '+err)
            }
            novi = false
        })

    },
    mark_inbound: function(scan_datetime, rmk, fail, arrived){
        var active  = this.active

        if (arrived){
            this.__segments[this.active]['a_arr'] = scan_datetime
        }
        if (rmk !== undefined) {
            this.__segments[this.active]['rmk'].push(rmk)
        }

        if (fail) {
            this.__segments[active]['st'] = 'FAIL'
            this.deactivate()
            var active = this.__segments[active]['par']
            this.active = active
        }
    },

    mark_outbound: function(scan_datetime, rmk, fail, departed){
        if (departed){
            this.__segments[this.active]['a_dep'] = scan_datetime
        }

        if (rmk !== undefined) {
            this.__segments[this.active]['rmk'].push(rmk)
        }

        if (fail) {
            this.__segments[this.active]['st'] = 'FAIL'
            this.deactivate()
            var active = this.__segments[this.active]['par']
            this.active = active
        }
        else {
            this.__segments[this.active]['st'] = 'REACHED'
            var active = this.lookup({
                'par': this.__segments[this.active]['idx'],
                'st': 'FUTURE'
            })
            this.active = active
            this.__segments[this.active]['st'] = 'ACTIVE'
        }
    },

    make_new: function(connection, intermediary, source){
        if (source === undefined || source === null){
            source = this.active
        }
        var segment = {}

        for (var key in this.__segments[this.active]){
            segment[key] = this.__segments[this.active][key]
        }
        segment['conn'] = connection
        segment['dst'] = intermediary
        segment['st'] = 'REACHED'
        segment['idx'] = this.__segments.length
        this.__segments.push(segment)
        this.active = segment['idx']
    },

    make_new_blank: function(src, dst, cid, sdt){
        var segment = {
            'src': src,
            'dst': dst,
            'conn': cid,
            'p_arr': null,
            'a_arr': null,
            'p_dep': null,
            'a_dep': sdt,
            'cst': this.__segments[this.active]['cst'],
            'st': 'REACHED',
            'rmk': [],
            'par': this.active,
            'sol': this.__segments[this.active]['sol'],
            'pdd': this.__segments[this.active]['pdd']
        }
        segment['idx'] = this.__segments.length
        this.__segments.push(segment)
        var active = segment['idx']
        this.active = active
    },

    mark_termination: function(msg){
        this.__segments[this.active]['rmk'].push(msg)
        this.__segments[this.active]['st'] = 'FAIL'
    },

    parse_inbound: function(location, scan_datetime){
        if (this.active == null){
            var err = 'ERROR (parse_inbound): No active nodes found'
            console.error(err)
            throw err
        }
        a_seg = this.__segments[this.active]

        if (a_seg['src'] === location){
            if (a_seg['a_arr'] !== null){
                console.log('Duplicate inbound')
                return true
            }
            if (a_seg['p_arr'] >= scan_datetime){
                this.mark_inbound(scan_datetime, undefined, false, true)
                return true
            } else if (scan_datetime < a_seg['p_dep']){
                this.mark_inbound(scan_datetime, 'WARN_LATE_ARRIVAL', false, true)
                return true
            } else {
                this.mark_inbound(
                    scan_datetime, 'FAIL_LATE_ARRIVAL', true, true)
            }
        }
        else{
            if (a_seg['a_arr'] !== null){
                this.mark_inbound(scan_datetime, 'UNEXPECTED_INBOUND', true, false)
                this.make_new(null, location, a_seg['idx'])
            } else {
                this.mark_inbound(
                    scan_datetime, 'LOCATION_MISMATCH', true, false)
            }
        }
        return false
    },

    parse_outbound: function(location, connection, scan_datetime){
        if (this.active == null) {
            console.error('ERROR (parse_outbound): No active nodes found')
            return
        }

        a_seg = this.__segments[this.active]

        // console.log(JSON.stringify(a_seg))
        // console.log('location '+ location)
        // console.log('connection '+ connection)
        // console.log('scan_datetime '+ scan_datetime)

        if (a_seg['a_arr'] === null){
            this.mark_inbound(scan_datetime, 'UNEXPECTED_OUTBOUND', true, false)
        } else if (a_seg['src'] === location){
            if (a_seg['conn'] === connection){
                if (a_seg['p_dep'] >= scan_datetime){
                    this.mark_outbound(scan_datetime, undefined, false, true)
                    return true
                }
                else if ((a_seg['p_dep'] - scan_datetime) < 24 * 3600){
                    this.mark_outbound(
                        scan_datetime, 'WARN_LATE_DEPARTURE', false, true)
                    return true
                }
                else{
                    this.mark_outbound(
                        scan_datetime, 'CONNECTION_MISMATCH', true, false)
                }
            } else {
                this.mark_outbound(
                    scan_datetime, 'CONNECTION_MISMATCH', true, false)
            }
        }
        else {
            this.mark_outbound(
                scan_datetime, 'LOCATION_MISMATCH', true, false)
            this.make_new(null, location, a_seg['idx'])
        }
        return false
    },

    lookup: function(kwargs){
        idx = null
        this.__segments.forEach(function(segment, index){
            if (Utils.match(segment, kwargs)){
                idx = index
            }
        })
        return idx
    },

    value: function(){
        return this.__segments
    }
})



exports.Parser = Parser;
