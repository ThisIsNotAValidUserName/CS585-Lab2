# CS585-Lab2
Solution for BU CS585 Lab2 2018 Fall<br>
## **Requirements**
1. For each shape image, determine its background color and label each and every shape blobs according to their color. <br>
2. For each shape blob, implement a border following algorithm to find its outermost contour (i.e. the border pixels) and compare it with OpenCV's "findContours" function. <br>
3. For each shape blob, classify its border pixels (in other words, segment its outermost contour) into three types: borders against the background, borders against another shape blob and borders that are a border of the whole image. <br>
4. (Optional) You may also segment the border according to their convexity, i.e. find out all convex segments and concave segments. This may help you analyze its shape type. <br>
5. For each shape blob, come up with an algorithm that can recognize its shape type (square, circle, or triangle). <br><br>
The code is written in C++ with OpenCV 3.3.0
