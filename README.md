# Smart Card Random Number Generator
It turns out that some smart cards have an implementation of the `GET CHALLENGE` instruction defined in the ISO 7816-4 standard. This means we can get random bytes out of the card without any software modifications!

The exact method by which the random data is generated is, of course, unknown (and may differ across different cards), but samples collected from one of my SIM cards pass all tests offered by [dieharder](https://webhome.phy.duke.edu/~rgb/General/dieharder.php). Note that the samples included in the `/sample` directory are marked with the ATR of the card they were collected from.

**WARNING: Obtaining large amounts of data may potentially degrade the card, so don't break your cards and consider yourself warned!**

![Image generated from collected random data. Looks like noise.](./sample/ATR3B9E95801FC78031E073FE211B66D00217F61100F1.png?raw=true)
