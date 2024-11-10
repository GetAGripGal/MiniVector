# Bad Apple Demo

## Details
One of the things I wanted to get working is the famous Touhou Project song 'Bad Apple' displayed on MiniVector.
I my managed to achieve this using a set of python scripts that transformed the frames into a set of points. The result of this project is stored in `test/animations/badapple.mv`.
In order to encode the frames each frame is reserved 800 points. Meaning each 800 frame is another frame.

## Result
<iframe width="560" height="315" src="https://www.youtube.com/embed/GKYqRum3V_0?si=jNDFF7cTJBmohi52" title="YouTube video player" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share" referrerpolicy="strict-origin-when-cross-origin" allowfullscreen></iframe>

## How to run
The resulting points are stored in `test/animations/badapple.mv`.
For accuracy it should be run in 30fps and 800 instructions per frame:

```bash
nohup minivector -fr 30 -e 800 &
cat ./test/animations/badapple.mv >> /tmp/mv_pipe
```