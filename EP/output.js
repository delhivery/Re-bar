var fs = require('fs');
var util = require('util');
var prompt = require('prompt');

var PATH = 'tests/%s'
var EPPATH = '/home/kuldeeprishi/projects/Re-bar/expresso/tests/%s'

function readOutput (wbn) {
    var path = util.format(PATH, wbn)
    data = fs.readFileSync(path)
    data = JSON.parse(data.toString())
    console.log('NODE OUTPUT')
    console.log('----------------------------')
    data.forEach( function(elem, index) {
        console.log(elem['idx'] + '\t' +elem['st'] + '\t' +elem['src'] + '\t' +elem['dst'] + '\t'+ elem['conn'] +'\t' +elem['rmk'])
    });

    console.log('')

    try {
        exp_data = fs.readFileSync(util.format(EPPATH, wbn))
        exp_data = JSON.parse(exp_data.toString())
        console.log('PYTHON OUTPUT')
        console.log('----------------------------')
        exp_data.forEach( function(elem, index) {
            console.log(elem['idx'] + '\t' +elem['st'] + '\t' +elem['src'] + '\t' +elem['dst'] + '\t'+ elem['conn'] +'\t' +elem['rmk'])
        });

    } catch(err) {
        console.log('****************************')
        console.log('GENERATE FILE WITH EXPRESSO')
        console.log('****************************')
    }

}


prompt.start();

prompt.get(['waybill'], function (err, result) {
  console.log('  waybill: ' + result.waybill);
    readOutput(result.waybill)
});
