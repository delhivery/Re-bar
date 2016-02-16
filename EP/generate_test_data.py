import copy
import requests
import base64
import json
import os
import random

PACKAGE_URI = 'https://hq.delhivery.com/api/p/info/wbns/.json'
HEADERS = {'Authorization': 'Token 1113c14cce8e165a763777257d8cf6652b448f0b'}


def get_data(wbns):
    if not isinstance(wbns, list):
        wbns = [wbns]
    wbns = map(str, wbns)
    req = requests.get(PACKAGE_URI, headers=HEADERS,
                       params={'wbns': ','.join(wbns)},
                       verify=False)
    return req.json()


def generate_pkg_life(package):
    raw_stream = []
    for scan in package.get('s', []):
        pkg = copy.deepcopy(package)
        del pkg['s']
        del pkg['cs']
        pkg['cs'] = scan
        raw_stream.append(pkg)
    return raw_stream


def encode_payload(payload):
    return base64.b64encode(json.dumps(payload))


def get_kinesis_record(payload):
    record = {}
    record['kinesis'] = {}
    record['kinesis']["eventVersion"] = '1.0'
    record['kinesis']["eventID"] = 'shardId-000000000046:49555326830316916618206437982542251972691333428371522274'
    record['kinesis']["sequenceNumber"] = '49555326830316916618206437982542251972691333428371522274'
    record['kinesis']["eventSourceARN"] = 'arn:aws:kinesis:us-east-1:190020191201:stream/Package.info'
    record['kinesis']["partitionKey"] = '253511779831'
    record['kinesis']["eventSource"] = 'aws:kinesis'
    record['kinesis']["invokeIdentityArn"] = 'arn:aws:iam::190020191201:role/lambda_kinesis_role'
    record['kinesis']["kinesisSchemaVersion"] = '1.0'
    record['kinesis']["awsRegion"] = 'us-east-1'
    record['kinesis']["eventName"] = 'aws:kinesis:record'
    record['kinesis']["data"] = encode_payload(payload)
    return record


def generate_kinesis_stream(raw_stream):
    stream = {}
    stream['Records'] = []
    for raw in raw_stream:
        stream['Records'].append(get_kinesis_record(raw))
    return stream


def write_to_file(data):
    filename = 'event.json'
    if os.path.exists(filename):
        os.remove(filename)

    with open(filename, 'w') as fl:
        fl.write(json.dumps(data, indent=4))


def createTestData(wbns):
    if not isinstance(wbns, list):
        wbns = [wbns]
    wbns = map(str, wbns)

    pkg_dict = {}

    packages = get_data(wbns)

    print 'Found {} package'.format(len(packages))

    for package in packages:
        wbn = package.get('wbn')
        pkg_dict[wbn] = generate_pkg_life(package)

    # shuffle kinesis_data
    kinesis_data = []

    while pkg_dict:
        pkg = random.choice(pkg_dict.keys())
        pkg_scans = pkg_dict.get(pkg)
        if pkg_scans:
            kinesis_data.append(pkg_scans.pop(0))
        else:
            del pkg_dict[pkg]

    kinesis_stream = generate_kinesis_stream(kinesis_data)
    write_to_file(kinesis_stream)

if __name__ == '__main__':
    wbn = raw_input('Enter waybill: ')
    createTestData(wbn)
