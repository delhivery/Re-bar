/**
 * Exposes a reader for scans off kinesis stream
 *
 * @module reader
 */

var Q = require('q');
var Parser = require('./parser').Parser;
var Utils = require('./utils')

MODES = ['RCSP', 'STSP']


/**
 * Class to read a package scan and load the appropriate graph
 *
 * @class ScanReader
 */
var ScanReader = Utils.Class({
    initialize: function(client, host, port, store) {
        this.__client = client
        this.__parser = new Parser()
        this.__host = host
        this.__port = port
        this.__waybill = null
        this.__store = store
        this.__data = []
    },

    /**
     * Read a scan and update the graph as needed
     *
     * @method read
     * @params {Object} scan_dict
     */
    read: function(scan_dict){
        var deferred = Q.defer()
        this.__waybill = scan_dict['wbn']

        if (scan_dict['ivd'] != null){
            try{
                scan_dict['cs']['sd'] = Utils.iso_to_seconds(scan_dict['cs']['sd'])
                scan_dict['pdd'] = Utils.iso_to_seconds(scan_dict['pdd'])
            } catch (err) {
                console.error('Error: BAD datetime '+ err)
                deferred.resolve(false)
            }

            var center = null

            try{
                center = scan_dict['cs']['sl']
                Utils.center_name_to_code(center)
                center = scan_dict['cn']
                Utils.center_name_to_code(center)
            } catch(err) {
                console.error('Error: BAD Center '+ err)
                this.__parser.mark_termination(err)
                deferred.resolve(false)
            }

            var pid = scan_dict['cs']['pid']
            var cid = scan_dict['cs']['cid']
            var ps = scan_dict['cs']['ps']

            if (['+C', '<C'].indexOf(scan_dict['cs']['act']) >= 0){
                if (cid === undefined && pid !== undefined){
                    scan_dict['cs']['cid'] = scan_dict['cs']['pid']
                    scan_dict['cs']['pri'] = true
                }
            } else {
                scan_dict['cs']['pri'] = eval(pid === ps)
            }

            scan_dict['cs']['sl'] = Utils.center_name_to_code(scan_dict['cs']['sl'])
            scan_dict['cn'] = Utils.center_name_to_code(scan_dict['cn'])

            var scan = {
                'src': scan_dict['cs']['sl'],
                'dst': scan_dict['cn'],
                'sdt': scan_dict['cs']['sd'],
                'pdd': scan_dict['pdd'],
                'act': scan_dict['cs']['act'] || null,
                'cid': scan_dict['cs']['cid'] || null,
            }

            if (scan_dict['cs']['pri'] === undefined){
                scan['pri'] = false
            } else {
                scan['pri'] = scan_dict['cs']['pri']
            }

            var self = this

            var _internal_func = function (self) {
                var def = Q.defer()


                if (['<L', '<C'].indexOf(scan['act']) >= 0 && scan['pri'] === true){
                    success = self.__parser.parse_inbound(scan['src'], scan['sdt'])
                    if (success != true){
                        createPromise = self.create(scan['src'], scan['dst'], scan['sdt'], scan['pdd'])
                        createPromise.then(function(result){
                            def.resolve(result)
                        })
                    } else {
                        def.resolve(true)
                    }

                } else if  (['+L', '+C'].indexOf(scan['act']) >= 0) {
                    success = self.__parser.parse_outbound(
                        scan['src'], scan['cid'], scan['sdt'])
                    if (success === false) {
                        predictResponse = self.predict(scan)

                        predictResponse.then(function(result){
                            def.resolve(result)
                        })
                    } else {
                            def.resolve(true)
                        }

                } else {
                    def.resolve(false)
                }
                return def.promise
            }

            response = this.load(scan['src'], scan['dst'], scan['sdt'], scan['pdd'])



            response.then(function(result){
                internalPromise = _internal_func(self)

                internalPromise.then(function(){
                storePromise = Utils.store_to_source(self.__waybill, self.__parser.value())
                storePromise.then(function(){
                    deferred.resolve(true)
                })
            })

            })
        } else {
            deferred.resolve(false)
        }
        return deferred.promise
    },

    /**
     * Returns status of graph on parsing edge
     *
     * @method data
     */
    data: function(){
        return self.__data
    },

    /**
     * Get or create graph data
     *
     * @method load
     * @params {String} src
     * @params {String} src
     * @params {Integar} sdt scan datetime
     * @params {Integar} pdd promise datetime
     */
    load: function(src, dst, sdt, pdd){
        var deferred = Q.defer()
        var self = this
        dataPromise = Utils.load_from_source(this.__waybill)

        dataPromise.then(function(segments){
            if (segments.length > 0){
                self.__parser.add_segments(segments)
                deferred.resolve(true)
            } else {
                res = self.create(src, dst, sdt, pdd)
                res.then(function(result){
                    deferred.resolve(result)
                })
            }

        })
        return deferred.promise
    },

    /**
     * Make prediction from outbound connection to arrival at destination
     *
     * @method predict
     * @params {Object} data
     */
    predict: function(data){
        var deferred = Q.defer()
        var self = this
        c_dataPromise = this.__client.lookup(data['src'], data['cid'])
        c_dataPromise.then(function(c_data){
            if (c_data['connection'] !== undefined){
                var lat = data['sdt'] + c_data['connection']['dur']
                var itd = c_data['connection']['dst']
                self.__parser.make_new_blank(data['src'], itd, data['cid'], data['sdt'])
                res = self.create(itd, data['dst'], lat, data['pdd'])
                res.then(function(result){
                    self.__parser.arrival(null)
                    deferred.resolve(result)
                })
            } else {
                self.__parser.make_new_blank(
                    data['src'], null, data['cid'], data['sdt'])
                self.__parser.mark_termination( 'MISSING DATA: ' + data['cid'] )
                deferred.resolve(true)
            }
        })
        return deferred.promise
    },

    /**
     * Calls the underlying client to fletcher to populate graph data
     *
     * @method solve
     * @params {String} src
     * @params {String} src
     * @params {Integar} sdt scan datetime
     * @params {Integar} pdd promise datetime
     * @params {Object} kwargs
     */
    solve: function(src, dst, sdt, pdd, kwargs){
        var deferred = Q.defer()
        var mode = 0
        if (kwargs !== undefined && kwargs.hasOwnProperty('mode')){
            mode = kwargs['mode']
        }
        var soutPromise = this.__client.get_path(src, dst, sdt, pdd, mode)

        var self = this
        soutPromise.then(function(sout){
            if (sout['path'] !== undefined && sout['path'].length > 0){
                kwargs = {'pdd': pdd, 'sol': MODES[mode]}
                sout = Utils.mod_path(sout['path'], sdt, kwargs)
                self.__parser.add_segments(sout, true)
                self.__parser.arrival(sdt)
                deferred.resolve(true)
            } else {
                deferred.resolve(false)
            }
        })

        return deferred.promise;
    },

    /**
     * Create (sub)graph. RCSP attempted with fallback to STSP
     *
     * @method create
     * @params {String} src
     * @params {String} src
     * @params {Integar} sdt scan datetime
     * @params {Integar} pdd promise datetime
     */
    create: function(src, dst, sdt, pdd){
        var deferred = Q.defer()
        var that = this
        if (src !== undefined && dst !== undefined){
            response = that.solve(src, dst, sdt, pdd)

            response.then(function(result){
                if (result === false){
                    response2 = that.solve(src, dst, sdt, pdd, {"mode": 1})
                    response2.then(function(res){
                        deferred.resolve(res)
                    })
                } else {
                    deferred.resolve(true)
                }
            })
        }
        return deferred.promise;
    }
})



exports.ScanReader = ScanReader;
