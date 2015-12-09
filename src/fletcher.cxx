#include "kinesis/consumer.cxx"
#include <aws/core/Region.h>

int main() {
    Consumer consumer("Package.info", Aws::Region::US_EAST_1);
    consumer.get_shards();
}
