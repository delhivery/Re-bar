/**
 * Implements a client to fletcher
 *
 * @module client
 */

var net = require('net');
var Q = require('q');
var Utils = require('./utils')


/**
 * Convert a number to bytes to send across a socket as a single byte
 *
 * @method number_to_bytes
 * @params {Number} number
 * @return number conveted to bytes buffer
 */
function number_to_bytes(number) {
    var buff = new Buffer(1);
    buff.writeUInt8(number, 0);
    return buff;
}

/**
 * Converts an argument to bytes to send across a socket as string
 *
 * @method keyword_to_bytes
 * @params {String} keyword
 * @return keyword conveted to bytes buffer
 */
function keyword_to_bytes(keyword) {
    var buff = new Buffer(keyword, 'utf-8');
    return buff;
}

/**
 * Converts an arguments to bytes with a header prefix specifying its length
 *
 * @method param_to_bytes
 * @params {String} params
 * @return params conveted to bytes buffer
 */
function param_to_bytes(param) {
    param = '' + param;
    return number_to_bytes(param.length) + keyword_to_bytes(param);
}

/**
 * Convert a map of named arguments to re-bar supported protocol
 *
 * @method kwargs_to_bytes
 * @params {Object} kwargs
 * @return kwargs conveted to bytes buffer
 */
function kwargs_to_bytes(kwargs) {
    var data = new Buffer(0);

    for (var key in kwargs) {
        var value = kwargs[key];

        if (Number(value) === value) {
            if (value % 1 === 0) {
                data += keyword_to_bytes('INT');
            }
            else {
                data += keyword_to_bytes('DBL');
            }
        }
        else {
            data += keyword_to_bytes('STR');
        }

        data += param_to_bytes(key) + param_to_bytes(kwargs[key]);
    }
    return data;
}

/**
 * Converts a mode, command and named argument combination to re-bar supported protocol
 *
 * @method command_to_bytes
 * @params {String} mode
 * @params {Number} command
 * @params {Object} kwargs
 * @return command conveted to bytes buffer
 */
function command_to_bytes(mode, command, kwargs) {
    var m = number_to_bytes(mode),
        c = keyword_to_bytes(command),
        l = number_to_bytes(Object.keys(kwargs).length),
        k = kwargs_to_bytes(kwargs);

    var data = m + c + l + k;
    return data;
}


/**
 * Client to re-bar.
 *
 * @class Client
 */
var Client = Utils.Class({
    /**
     * Initializes a connection to re-bar
     *
     * @method initialize
     *
     * @params {String} host
     * @params {Number} port
     */
    initialize: function(host, port){
        this.HOST = host
        this.PORT = port
        this.__handler = new net.Socket();
        this.__handler.connect({'host': this.HOST, 'port': this.PORT})
    },

    /**
     * Reconnects the connection against server
     *
     * @method reconnect
     */
    reconnect: function(){
        this.__handler.connect({'host': this.HOST, 'port': this.PORT})
    },

    /**
     * Terminates the connection against server
     *
     * @method close
     */
    close: function(){
        this.__handler.end()
        this.__handler.destroy()
    },

    /**
     * Executes a command against server and returns the json response
     *
     * @method execute
     *
     * @params {Number} command
     * @params {String} mode
     * @params {Object} kwargs
     *
     * @return Returns a promise object which will resolve to a json object on completion
     */
    execute: function(command, mode, kwargs) {

        var deferred = Q.defer()
        if (mode === undefined){
            mode = 0
        }
        if (this.__handler.writable === false){
            this.reconnect()
        }
        var that = this
        this.__handler.write(command_to_bytes(mode, command, kwargs));
        var data = '';
        var length = 0

        this.__handler.on('data', function (chunk) {
            data += chunk
            that.__handler.end()

        });

        this.__handler.on('end', function () {
            if (data === ''){
                return
            }
            var json = {}
            var json_string = data.slice(4)

            try{
                var json = JSON.parse(json_string)
            } catch (err){
                console.log('-----------------------------------------')
                console.error('ERROR (execute): '+ err)
                console.log('json_string: '+json_string)
                console.dir(command_to_bytes(mode, command, kwargs))
                console.log('-----------------------------------------')
                console.log('')
                json = {}
            }
            data = ''
            that.close()
            deferred.resolve(json);
        });

        return deferred.promise;
    },

    /**
     * Add a vertex to solver
     *
     * @method add_vertex
     * @params {String} vertes
     *
     * @return Returns a promise object which will resolve to a json object on success
     */
    add_vertex: function(vertex){
        if (typeof(vertex) != 'string') {
            throw 'Vertices should be a code or a list of codes. Got '+vertex
        }
        return this.execute("ADDV", undefined, {"code": vertex})
    },

    /**
     * Initializes a connection to re-bar
     *
     * @method add_vertices
     * @params {Array} vertes
     *
     * @return Returns a promise object which will resolve to a json object on success
     */
    add_vertices: function(vertices){
        var response = {}
        var dis = this
        vertices.forEach(function(vertex){
            response[vertex] = dis.add_vertex(vertex)
        })
        return response
    },

    /**
     *  Add an edge to solver. Parameters <br>
     *   [in]source: code for source vertex <br>
     *   [in]destination: code for destination vertex <br>
     *   [in]code: code for connection <br>
     *   [in]tip: time for inbound processing <br>
     *   [in]tap: time for aggregation processing <br>
     *   [in]top: time for outbound processing <br>
     *   [in]departure: time of departure <br>
     *   [in]duration: duration of connection traversal <br>
     *   [in]cost: cost of connection traversal <br>
     *
     * @method add_edge
     * @params {Object} kwargs
     *
     * @return Returns a promise object which will resolve to a json object on success
     */
    add_edge: function(kwargs){
        var source = kwargs['source'] || null
        var destination = kwargs['destination'] || null
        var code = kwargs['code'] || null
        var tip = kwargs['tip'] || null
        var tap = kwargs['tap'] || null
        var top = kwargs['top'] || null
        var departure = kwargs['departure'] || null
        var duration = kwargs['duration'] || null
        var cost = kwargs['cost'] || null

        _kwargs = {
            'src': source,
            'dst': destination,
            'conn': code,
            'tip': tip,
            'tap': tap,
            'top': top
        }

        if (departure === null && duration === null && cost === null){
            var command = "ADDC"
            var mode = undefined
            return this.execute(command, mode, kwargs)
        } else {
            kwargs['dep'] = departure
            kwargs['dur'] = duration
            kwargs['cost'] = parseFloat(cost)
            var command = "ADDE"
            var mode = undefined
            return this.execute(command, mode, kwargs)
        }
    },

    /**
     * Add edges to solver
     *
     * @method add_edges
     */
    add_edges: function(edges){
        var response = {}
        edges.forEach(function(edge){
            response[edge] = this.add_edge(edge)
        })
        return response
    },

    /**
     * Fetch attributes of edge in solver
     *
     * @method lookup
     * @params {String} source
     * @params {Integar} edge
     *
     * @return Returns a promise object which will resolve to a json object on success
     */
    lookup: function(source, edge){
        var command = "LOOK"
        var mode = 0
        var kwargs = {"src":source, "conn":edge}
        return this.execute(command, mode, kwargs)
    },

    /**
     * Find a path using solver
     *
     * @method get_path
     * @params {String} source
     * @params {String} destination
     * @params {String} t_start
     * @params {String} t_max
     * @params {String} mode
     *
     * @return Returns a promise object which will resolve to a json object on success
     */
    get_path: function(source, destination, t_start, t_max, mode){
        var deferred = Q.defer()
        if (mode == undefined)
            mode = 0

        var command = "FIND";
        var kwargs = {"src":source, "dst":destination, "beg":t_start, "tmax":t_max};
        this.execute(command, mode, kwargs).then(function (value) {
            deferred.resolve(value);
        });

        return deferred.promise;
    }
})


exports.Client = Client;
