#include <thread>

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

/*
Aws::Kinesis::Model::DescribeStreamOutcome Aws::Kinesis::KinesisClient::DescribeStream(
        const Aws::Kinesis::Model::DescribeStreamRequest& request) const {
    Aws::StringStream ss;
    ss << m_uri << "/";
    Aws::Client::JsonOutcome outcome = MakeRequest(ss.str(), request, Aws::Http::HttpMethod::HTTP_POST);

    if (outcome.IsSuccess()) {
        return Aws::Kinesis::Model::DescribeStreamOutcome(Aws::Kinesis::Model::DescribeStreamResult(outcome.GetResult()));
    }
    else {
        return Aws::Kinesis::Model::DescribeStreamOutcome(outcome.GetError());
    }
}*/


class Consumer {
    private:
        Aws::Kinesis::KinesisClient client;
        std::string stream;

    public:

        //Consumer(Aws::Region region=Aws::Region::AP_SOUTHEAST_1) {
        //}

        void get_records(std::string shard_iterator) {

            while(true) {
                Aws::Kinesis::Model::GetRecordsRequest get_records_request;
                get_records_request.SetShardIterator(shard_iterator);
                get_records_request.SetLimit(100);

                auto get_records_result = client.GetRecords(get_records_request).GetResult();
                auto records = get_records_result.GetRecords();
                std::this_thread::sleep_for(std::chrono::milliseconds(10000));
                shard_iterator = get_records_result.GetNextShardIterator();

                if (shard_iterator == "") {
                    break;
                }
            }
        }

        void iterate_shards(std::vector<Aws::Kinesis::Model::Shard> shards) {
            std::vector<std::string> shard_iterators;
            for (auto shard: shards) {
                std::string shard_iterator;
                Aws::Kinesis::Model::GetShardIteratorRequest get_shard_iterator_request;
                get_shard_iterator_request.SetStreamName(stream);
                get_shard_iterator_request.SetShardId(shard.GetShardId());
                get_shard_iterator_request.SetShardIteratorType(Aws::Kinesis::Model::ShardIteratorType::TRIM_HORIZON);

                auto get_shard_iterator_result = client.GetShardIterator(get_shard_iterator_request);
                shard_iterator = get_shard_iterator_result.GetResult().GetShardIterator();
                shard_iterators.push_back(shard_iterator);
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

            while(true) {
                std::vector<Aws::Kinesis::Model::Shard> shards;
                Aws::Kinesis::Model::DescribeStreamRequest describe_stream_request;
                describe_stream_request.SetStreamName(stream);
                std::string start_shard_id = "";

                do {
                    describe_stream_request.SetExclusiveStartShardId(start_shard_id);
                    auto describe_stream_result = client.DescribeStream(describe_stream_request).GetResult();

                    for (auto shard: describe_stream_result.GetStreamDescription().GetShards()) {
                        shards.push_back(shard);
                    }

                    if (describe_stream_result.GetStreamDescription().GetHasMoreShards() and shards.size() > 0) {
                        start_shard_id = shards[shards.size() - 1].GetShardId();
                    }
                    else {
                        start_shard_id = "";
                    }
                } while(start_shard_id != "");

                for (auto shard: shards) {
                }
            }
        }
};
