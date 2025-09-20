# JazzyOrderBook
***Requirement***<br/>
 - Can receive order by order updates.
 - generate Price ordered to of book (N levels) with volume aggregation.
 - (no requirement for volume aggregation outside of Tob of Book)
 
***Goal***<br/>
to replicate and if possibl improve on the order book ideas describe in cpp con video<br/>
do this by:<br/>
- *Replicatiopn*
    - ...
- *Possible Enhancement*
    - ...

***Rough layout***
```cpp
enum class UpdteType { INSERT, MODIFY, DELETE, UNKNOWN };
enum class Side { BUY, SELL, UNKNOWN };

using OrderID = char[12];
struct Price
{
 unsigned price
 short dp;
};

struct OrderUpdate
{
 UpdateType updateType;
 Price price;
 unsigned qty;
 Side side;
};
struct StaticData
{
  Price high, low, close;
  short tick_size;
};

struct Order
{
 OrderID id;
 unsigned ticks;
 unsigned qty;
 Side side;
};
struct OrderEntyLevel
{
  unsigned ticks;
  unsinged qty;
  Side side;
};
```
 
```mermaid
flowchart TD
  A[***Market Update***<br/> - Price<br/> - QTY<br/> - Update Type: New/Update/Delete] --> C{***Price -> Tick Conversion***<br/>Use as index}
  B[***Static Data***<br/>Tick Size<br/>High<br/>Low<br/>Close<br/>] --> C
  C --> E[***Flat array&lt;size_t size&gt; - Top of Book***<br/> - qty, num_orders, best_order_id/head  in a POD struct<br/> - Optionally Aligned and paded to avoid false sharing if multiple threads will touch adjacent levels.]
  E -->F{Back of book queue}
  F --> H{Processing thred}
  H --> H
  H <--> G[Back of book data<br/>Hash Map keyed on order id]
```

# Compre/Benchmark against some existing techniques
