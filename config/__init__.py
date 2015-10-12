MONGO_HOSTS = [
    'mongo-fap-01.internal.sdb',
    'mongo-fap-02.internal.sdb'
]

MONGO_URI_OPTS = [
    'replicaSet=FAP',
    'readPreference=secondaryPreferred'
]

MONGO_URI = 'mongodb://{}/?{}'.format(
    ','.join(MONGO_HOSTS),
    '&'.join(MONGO_URI_OPTS)
)

DISQUE_HOSTS = [
    'disque-fap-01.internal.sdb:7711',
    'disque-fap-02.internal.sdb:7711',
    'disque-fap-03.internal.sdb:7711',
    'disque-fap-04.internal.sdb:7711',
    'disque-fap-05.internal.sdb:7711',
    'disque-fap-06.internal.sdb:7711',
    'disque-fap-07.internal.sdb:7711',
    'disque-fap-08.internal.sdb:7711',
]

FAP_QUEUE = 'fap_fap_fap'

JOBS_TO_FETCH = 10
