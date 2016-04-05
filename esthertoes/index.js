var aws = require('aws-sdk')
var s3 = new aws.S3()
var esDomain = {
  endpoint: 'search-expath-5x6dtx75lntwuimllxrygn2mje.us-east-1.es.amazonaws.com',
  region: 'us-east-1',
  index: 'sierra',
  docType: 'segment'
}

var endpoint = new aws.Endpoint(esDomain.endpoint)
var creds = new aws.EnvironmentCredentials('AWS')

function postDocumentToES (segments, waybill, context) {
  var data = []
  segments.forEach(function (segment) {
    segment['wbn'] = waybill
    if (segment.idx !== 0 && segment.idx !== '0') {
      data.push({update: {_index: esDomain.index, _type: esDomain.docType, _id: waybill + '' + segment.idx}})
      data.push({doc: segment, doc_as_upsert: true})
    }
  })

  var req = new aws.HttpRequest(endpoint)
  req.path = '/_bulk'
  req.region = esDomain.region
  req.body = data
  req.headers['presigned-expires'] = false
  req.headers['Host'] = endpoint.host

  var signer = new aws.Signers.V4(req, 'es')
  signer.addAuthorization(creds, new Date())

  var send = new aws.NodeHttpClient()
  send.handleRequest(req, null, function (httpResp) {
    var body = ''

    httpResp.on('data', function (chunk) {
      body += chunk
    })

    httpResp.on('end', function (chunk) {
      console.log('Added segments added to ES')
      context.succeed()
    })
  }, function (err) {
    console.log('Error: ' + err)
    context.fail()
  })
}

exports.handler = function (event, context) {
  event.Records.forEach(function (record) {
    var bucket = record.s3.bucket.name
    var objKey = decodeURIComponent(record.s3.object.key.replace(/\+/g, ' '))

    s3.getObject({Bucket: bucket, Key: objKey}, function (err, data) {
      if (err) {
        console.log(err)
        context.fail()
      } else {
        var segments = JSON.parse(data.Body.toString('UTF-8'))
        postDocumentToES(segments, objKey, context)
      }
    })
  })
}
