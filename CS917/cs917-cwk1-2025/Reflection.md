REFLECTION.md

In this coursework, I managed to build each part in a structured way one by one. Initially, I developed Part A to make sure that all the basic functionality works as expected on the cryptocompare_btc.csv dataset. Then it was straightforward to expand the existing logic to Part B by incorporating exception handling and defensive checks. Part C was the most interesting part to develop because it required me to leverage the moving average crossover strategy, so it was pretty engaging to reason about the lists, date filtering, and so on. Finally, with the finished logic from the Parts A, B, and C, it was quite easy to integrate all that logic into an Investment class in Part D, and incorporate the prediction and trend classification methods.

I felt that this iterative methodology allowed me to get a comprehensive idea of the basis on which I would evolve functions into object-oriented designs. As for the hardest aspect of this module, I have identified debugging date conversions and making sure that the filtering was functional with UTC timestamps. 

I made use of ChatGPT (GPT-5) on 20 October 2025 for debugging only. Specifically, including:
1. Making date parsing more robust and handling invalid date formats
2. Ensuring filter_by_date returns are checked before using the result
3. Improving get_date_range to handle missing or invalid timestamps
