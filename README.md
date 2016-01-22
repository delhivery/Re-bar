# Re-bar, Expected Path and Metrics
Re-bar is an ongoing experiment to build routing solutions for shipments moving across the Delivery Last-Mile Network. Its goal is to come up with ideal recommendations for paths a shipments or bags should take in order to reach their destinations within SLAs while minimizing costs and satisfying other operational criteria.

Currently (As of Dec 8), the project has been rebooted while being complely written in C++ in order to increase performance and to support new algorithms to come up with Multi-Criteria Shortest Paths. Currently, only the new algorithm has been implemented. It still requires modules to read data from/push data to Kinesis/Express and a parser to store historically predicted paths and update their states.

The legacy version(Release 1.0.0) achieves shortest time paths with no constraints. However, it also ships with a Kinesis Consumer, a parser to update historical predictions of EP and a mongo producer to store this information. mongo-connector is recommended to sync data from MongoDB to ElasticSearch for analytics.


# What is Expected Path
Consider our Express Network. There are a list of delivery centers and vehicles that move shipments from one center to another. These vehicles mostly operate on fixed schedules. Let us say a shipment has to move from center A to center B. Given that A might be connected to a lot of other centers which in turn connect to B, the number of choices of moving this shipment is incredibly large. Additionally, each shipment has to be delivered within a guaranteed SLA. Finally, since vehicles operate in time windows, it is possible that the shipment might have to wait at each center it visits before it can connect via an outbound vehicle to the next center.

This constraint alone limits the centers, center A should move the shipment to in order to deliver the shipment in time. However, there could be additional constraints such as capacities of vehicles, promised volumes per vehicle etc

EP tries to recommend a route which the shipment should thus take, considering all the above criteria while minimizing the shipping cost incrued while moving the shipment.

# Details!
Re-bar consists of following moving parts:
- [ ] Expressor - An ETL tool to load and parse paths based on scans[#WIP]
- [x] Fletcher - Expected path prediction minimizing the cost against time constraints


# SetUp
Re-bar has moved to cmake for its build system. Following are the individual project dependencies:
- Fletcher:
    * [boost](http://www.boost.org/)
    * [asio](http://think-async.com/Asio/)
- Expresso:
    * [lambda](http://docs.aws.amazon.com/lambda/latest/dg/welcome.html)

You can use cmake to build the project.
- Fletcher
    <pre>
      #$ git clone git@github.com:delhivery/Re-bar.git
      #$ cd Re-bar/build
      #$ cmake ..
      #$ make
      #$ sudo make install
    </pre>
