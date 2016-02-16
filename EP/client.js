// require('buffer');
var net = require('net');
var Q = require('q');


function number_to_bytes(number) {
    var buff = new Buffer(1);
    buff.writeUInt8(number, 0);
    return buff;
}

function keyword_to_bytes(keyword) {
    var buff = new Buffer(keyword, 'utf-8');
    return buff;
}

function param_to_bytes(param) {
    param = '' + param;
    return number_to_bytes(param.length) + keyword_to_bytes(param);
}

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

function command_to_bytes(mode, command, kwargs) {
    var m = number_to_bytes(mode),
        c = keyword_to_bytes(command),
        l = number_to_bytes(Object.keys(kwargs).length),
        k = kwargs_to_bytes(kwargs);

    var data = m + c + l + k;
    return data;
}


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
var Client = Class({
    initialize: function(host, port){
        this.HOST = host
        this.PORT = port
        this.__handler = new net.Socket();
        this.__handler.connect({'host': this.HOST, 'port': this.PORT})
    },

    reconnect: function(){
        this.__handler.connect({'host': this.HOST, 'port': this.PORT})
    },

    close: function(){
        this.__handler.end()
        this.__handler.destroy()
    },

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
            // console.log('============ON==============')
            data += chunk
            // console.log('Got chunk Length '+ chunk.length)
            // console.log('Data Length '+ data.length)
            // console.log(data)
            that.__handler.end()

        });

        this.__handler.on('end', function () {
            if (data === ''){
                return
            }
            // console.log('**********END*********')
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
            // console.log('')
            deferred.resolve(json);
        });

        return deferred.promise;
    },

    add_vertex: function(vertex){
        if (typeof(vertex) != 'string') {
            throw 'Vertices should be a code or a list of codes. Got '+vertex
        }
        return this.execute("ADDV", undefined, {"code": vertex})
    },

    add_vertices: function(vertices){
        var response = {}
        var dis = this
        vertices.forEach(function(vertex){
            response[vertex] = dis.add_vertex(vertex)
        })
        return response
    },

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

    add_edges: function(edges){
        var response = {}
        edges.forEach(function(edge){
            response[edge] = this.add_edge(edge)
        })
        return response
    },

    lookup: function(source, edge){
        var command = "LOOK"
        var mode = 0
        var kwargs = {"src":source, "conn":edge}
        return this.execute(command, mode, kwargs)
    },

    get_path: function(source, destination, t_start, t_max, mode){
        var deferred = Q.defer()
        if (mode == undefined)
            mode = 0

        var command = "FIND";
        var kwargs = {"src":source, "dst":destination, "beg":t_start, "tmax":t_max};
        // console.log(command, mode, kwargs)
        this.execute(command, mode, kwargs).then(function (value) {
            deferred.resolve(value);
        });

        return deferred.promise;
    }
})


exports.Client = Client;
