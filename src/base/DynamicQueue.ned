package inet.base;

simple DynamicQueue
{
    parameters:
        double procDelay @unit("s") = default(0s);
        @display("i=block/queue_s");
    gates:
        input queueIn @labels(IPv4Datagram);
        output queueOut @labels(IPv4Datagram);
}