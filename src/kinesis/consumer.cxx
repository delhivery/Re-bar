#include <iostream>
#include <thread>

#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/http/HttpClient.h>
#include <aws/core/http/HttpResponse.h>
#include <aws/core/http/HttpClientFactory.h>
#include <aws/core/utils/Outcome.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/kinesis/KinesisClient.h>
#include <aws/kinesis/KinesisRequest.h>
#include <aws/kinesis/model/DescribeStreamRequest.h>
#include <aws/kinesis/model/DescribeStreamResult.h>
#include <aws/kinesis/model/GetShardIteratorRequest.h>
#include <aws/kinesis/model/GetRecordsRequest.h>

#include <zmq.hpp>


class Consumer {
    private:
        Aws::Kinesis::KinesisClient client;
        Aws::Client::ClientConfiguration conf;
        std::string stream;

    public:

        Consumer(std::string stream, Aws::Region region=Aws::Region::AP_SOUTHEAST_1) : stream(stream) {
            std::cout << "Set stream and region" << std::endl;
            conf.region = region;
            conf.scheme = Aws::Http::Scheme::HTTPS;
            client = Aws::Kinesis::KinesisClient{conf};
        }

        void show_record(Aws::Kinesis::Model::Record record) {
            std::cout << "Record: " << record.GetData().GetUnderlyingData() << std::endl;
        }

        void get_records(std::string shard_iterator) {

            while(true) {
                Aws::Kinesis::Model::GetRecordsRequest get_records_request;
                get_records_request.SetShardIterator(shard_iterator);
                get_records_request.SetLimit(100);

                auto get_records = client.GetRecords(get_records_request);
                if (get_records.IsSuccess()) {
                    auto get_records_result = get_records.GetResult();
                    auto records = get_records_result.GetRecords();

                    for (auto const& record: records) {
                        std::thread threaded = std::thread([this, record](){ show_record(record); });
                        threaded.detach();
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
                    shard_iterator = get_records_result.GetNextShardIterator();

                    if (shard_iterator == "") {
                        break;
                    }
                }
                else {
                    auto get_records_error = get_records.GetError();
                    std::cout << "Unable to get records from shard iterator " << shard_iterator << ": ";
                    std::cout << get_records_error.GetMessage() << std::endl;
                    return;
                }
            }
        }

        void iterate_shards(std::vector<Aws::Kinesis::Model::Shard> shards) {
            std::vector<std::string> shard_iterators;

            for (auto const& shard: shards) {
                std::string shard_iterator;
                Aws::Kinesis::Model::GetShardIteratorRequest get_shard_iterator_request;
                get_shard_iterator_request.SetStreamName(stream);
                get_shard_iterator_request.SetShardId(shard.GetShardId());
                get_shard_iterator_request.SetShardIteratorType(Aws::Kinesis::Model::ShardIteratorType::TRIM_HORIZON);

                auto get_shard_iterator_result = client.GetShardIterator(get_shard_iterator_request);

                if (get_shard_iterator_result.IsSuccess()) {
                    shard_iterator = get_shard_iterator_result.GetResult().GetShardIterator();
                    shard_iterators.push_back(shard_iterator);
                }
                else {
                    std::cout << "Unable to get shard iterators: " << get_shard_iterator_result.GetError().GetMessage() << std::endl;
                }
            }

            std::thread *threads = new std::thread[shard_iterators.size()];

            for (std::size_t index = 0; index < shard_iterators.size(); index++) {
                auto shard_iterator = shard_iterators[index];
                threads[index] = std::thread([this, shard_iterator](){ get_records(shard_iterator); });
                // threads[index] = std::thread(&Consumer::get_records, *this, shard_iterator);
            }

            for (std::size_t index = 0; index < shard_iterators.size(); index++) {
                threads[index].join();
            }
        }

        void get_shards() {
            std::cout << "Trying to get shards" << std::endl;

            while(true) {
                std::vector<Aws::Kinesis::Model::Shard> shards;
                Aws::Kinesis::Model::DescribeStreamRequest describe_stream_request;
                describe_stream_request.SetStreamName(stream);
                std::string start_shard_id = "";

                do {
                    if (start_shard_id != "")
                        describe_stream_request.SetExclusiveStartShardId(start_shard_id);

                    auto describe_stream = client.DescribeStream(describe_stream_request);

                    if (describe_stream.IsSuccess()) {
                        auto describe_stream_result = describe_stream.GetResult();
                        for (auto const& shard: describe_stream_result.GetStreamDescription().GetShards()) {
                            shards.push_back(shard);
                        }

                        std::cout << shards.size() << " shards found." << std::endl;

                        if (describe_stream_result.GetStreamDescription().GetHasMoreShards() and shards.size() > 0) {
                            start_shard_id = shards[shards.size() - 1].GetShardId();
                        }
                        else {
                            start_shard_id = "";
                        }
                    } else {
                        auto describe_stream_error = describe_stream.GetError();
                        std::cout << "Unable to describe stream: " << describe_stream_error.GetMessage() << std::endl;
                    }
                } while(start_shard_id != "");

                iterate_shards(shards);
            }
        }
};
