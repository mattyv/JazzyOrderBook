# JazzyOrderBook
***Requirement***
    Can receive order by order updates. 
    generate Price ordered to of book (N levels) with volume aggregation. 
    (no requirement for volume aggregation outside of Tob of Book)
***Goal***
    to replicate and improve on the order book ideas describe in cpp con video
    do this by:
        - *replicate*
        - 
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
