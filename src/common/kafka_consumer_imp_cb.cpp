#include "kafka_consumer_imp_cb.h"
#include "MsgConsumer.h"

void  ExampleEventCb::event_cb(RdKafka::Event &event) {
    switch (event.type()) {
      case RdKafka::Event::EVENT_ERROR:
        if (event.fatal()) {
          std::cerr << "FATAL ";
          // run = false;
        }
        std::cerr << "ERROR (" << RdKafka::err2str(event.err())
                  << "): " << event.str() << std::endl;
        break;

      case RdKafka::Event::EVENT_STATS:
        std::cerr << "\"STATS\": " << event.str() << std::endl;
        break;

      case RdKafka::Event::EVENT_LOG:
        fprintf(stderr, "LOG-%i-%s: %s\n", event.severity(),
                event.fac().c_str(), event.str().c_str());
        if ( event.fac().c_str() == "MAXPOLL") {
          if( !MsgConsumer::get_instance()->Restart() ){
            exit(0);
          }
        }
        break;

      case RdKafka::Event::EVENT_THROTTLE:
        std::cerr << "THROTTLED: " << event.throttle_time() << "ms by "
                  << event.broker_name() << " id " << (int)event.broker_id()
                  << std::endl;
        break;

      default:
        std::cerr << "EVENT " << event.type() << " ("
                  << RdKafka::err2str(event.err()) << "): " << event.str()
                  << std::endl;
        break;
    }
  }


void ExampleRebalanceCb::part_list_print(
      const std::vector<RdKafka::TopicPartition *> &partitions) {
    for (unsigned int i = 0; i < partitions.size(); i++)
      std::cerr << partitions[i]->topic() << "[" << partitions[i]->partition()
                << "], ";
    std::cerr << "\n";
  }
void ExampleRebalanceCb::rebalance_cb(RdKafka::KafkaConsumer *consumer, RdKafka::ErrorCode err,
                    std::vector<RdKafka::TopicPartition *> &partitions) {
    std::cerr << "RebalanceCb: " << RdKafka::err2str(err) << ": ";

    part_list_print(partitions);

    if (err == RdKafka::ERR__ASSIGN_PARTITIONS) {
      consumer->assign(partitions);
    } else {
      consumer->unassign();
    }
  }




