"
Copyright (c) 2008 Vincent Geddes
Copyright (c) 2008 Luca Bruno

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the 'Software'), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
DEALINGS IN THE SOFTWARE.
"

"constants"

Number classMethod!
pi
	^ 3.1415926535897932384626433832795028841971693993751!

Number classMethod!
e
	^ 2.7182818284590452353602874713526624977572470937000!

"instance creation"

Number classMethod!
new
    self shouldNotImplement!


"testing"

Number method!
isNumber
    ^ true!

Number method!
isInteger
    ^ false!

Number method!
isSmallInteger
    ^ false!

Number method!
isLargeInteger
    ^ false!

Number method!
isFloat
    ^ false!

Number method!
isFraction
    ^ false!

Number method!
isZero
    ^ self = self zero!

Number method!
isNegative
    ^ self < self zero!

Number method!
isPositive
    ^ self >= self zero!

Number method!
isStrictlyPositive
    ^ self > self zero!

Number method!
sign
    self < self zero ifTrue: [^ -1].
    self > self zero ifTrue: [^  1].
    ^ 0!

"intervals"

Number method!
to: stop
    "Create an Interval between the receiver and stop"
    ^ Interval from: self to: stop!

Number method!
to: stop by: step
    "Create an Interval between the receiver and stop by step"
    ^ Interval from: self to: stop by: step!

Number method!
to: stop do: aBlock
    "Do aBlock from self to stop. Pass the counter to aBlock"
    self to: stop by: self unity do: aBlock!

Number method!
to: stop reverseDo: aBlock
    "Do aBlock decreasing self to stop. Pass the counter to aBlock"
    self to: stop by: self unity reverseDo: aBlock!

Number method!
to: stop by: step do: aBlock
    "Do aBlock increasing self to stop stepping by step. Pass the counter to aBlock"
    | i |
    i := self.
    step isPositive
	ifTrue: [
	    [ i <= stop ]
		whileTrue: [ aBlock value: i.
			     i := i + step ] ]
	ifFalse: [
	    [ i >= stop ]
		whileTrue: [ aBlock value: i.
			     i := i + step ] ]!

Number method!
to: stop by: step reverseDo: aBlock
    "Do aBlock decreasing self to stop stepping by step. Pass the counter to aBlock"
    | i |
    i := self.
    step isPositive
	ifTrue: [
	    [ i >= stop ]
		whileTrue: [ aBlock value: i.
			     i := i - step ] ]
	ifFalse: [
	    [ i <= stop ]
		whileTrue: [ aBlock value: i.
			     i := i - step ] ]!


"coercing"

Number method!
asFraction
    ^ Fraction numerator: self denominator: 1!

Number method!
asNumber
    ^ self!

Number method!
asInteger
    self subclassResponsibility!

Number method!
asFloat
    self subclassResponsibility!

Number method!
zero
    ^ self subclassResponsibility!

Number method!
generality
    self subclassResponsibility!

Number method!
unity
    self subclassResponsibility!

Number method!
coerce: aNumber
    ^ aNumber!


"arithmetic"

Number method!
negated
    self ~= self zero
		ifTrue: [ ^ self zero - self ]!

Number method!
abs
    self < self zero
		ifTrue: [ ^ self negated ]
		ifFalse: [ ^ self ]!

Number method!
reciprocal
    "Answer the reciprocal number of self"
    ^ 1 / self!

Number method!
+ aNumber
    "Coerce aNumber and do the sum"
    ^ self generality > aNumber generality
		ifTrue: [ self + (self coerce: aNumber) ]
		ifFalse: [ (aNumber coerce: self) + aNumber ]!

Number method!
- aNumber
    "Coerce aNumber and do the difference"
    ^ self generality > aNumber generality
		ifTrue: [ self - (self coerce: aNumber) ]
		ifFalse: [ (aNumber coerce: self) - aNumber ]!

Number method!
* aNumber
    "Coerce aNumber and do multiplication"
    ^ self generality > aNumber generality
		ifTrue: [ self * (self coerce: aNumber) ]
		ifFalse: [ (aNumber coerce: self) * aNumber ]!

Number method!
/ aNumber
    "Coerce aNumber and do division"
    aNumber isZero
	ifTrue: [ self zeroDivide ].

    ^ self generality > aNumber generality
        ifTrue: [ self / (self coerce: aNumber) ]
        ifFalse: [ (aNumber coerce: self) / aNumber ]!

Number method!
// aNumber
    "Do division then answer the quotient floor"
    ^ (self / aNumber) floor!

Number method!
\\ aNumber
    "Coerce aNumber and do modulo"
    ^ self generality > aNumber generality
		ifTrue: [ self \\ (self coerce: aNumber) ]
		ifFalse: [ (aNumber coerce: self) \\ aNumber ]!

Number method!
quo: aNumber
    "Do division then answer the truncated quotient towards zero"
    ^ (self / aNumber) truncated!

Number method!
rem: aNumber
    "Do division then answer the truncated remainder towards zero"
    self notYetImplemented!


"comparing"

Number method!
compare: aNumber
    "Return 1 if self > aNumber, 0 if = aNumber, -1 if < aNumber"
    self > aNumber ifTrue: [ ^ 1 ].
    self = aNumber ifTrue: [ ^ 0 ].
    self < aNumber ifTrue: [ ^ -1 ]!

Number method!
< aNumber
    "Coerce aNumber and compare"
    ^ self generality > aNumber generality
		ifTrue: [ self < (self coerce: aNumber) ]
		ifFalse: [ (aNumber coerce: self) < aNumber ]!

Number method!
> aNumber
    "Coerce aNumber and compare"
    ^ self generality > aNumber generality
		ifTrue: [ self > (self coerce: aNumber) ]
		ifFalse: [ (aNumber coerce: self) > aNumber ]!

Number method!
<= aNumber
    "Coerce aNumber and compare"
    ^ self generality > aNumber generality
		ifTrue: [ self <= (self coerce: aNumber) ]
		ifFalse: [ (aNumber coerce: self) <= aNumber ]!

Number method!
>= aNumber
    "Coerce aNumber and compare"
    ^ self generality > aNumber generality
		ifTrue: [ self >= (self coerce: aNumber) ]
		ifFalse: [ (aNumber coerce: self) >= aNumber ]!

Number method!
= aNumber
    "Coerce aNumber and compare equality"
    aNumber isNumber
		ifFalse: [ ^ false ].
    ^ self generality > aNumber generality
		ifTrue: [ self = (self coerce: aNumber) ]
		ifFalse: [ (aNumber coerce: self) = aNumber ]!

Number method!
~= aNumber
    "Coerce aNumber and compare inequality"
    ^ self generality > aNumber generality
		ifTrue: [ self ~= (self coerce: aNumber) ]
		ifFalse: [ (aNumber coerce: self) ~= aNumber ]!



"signaling"

Number method!
zeroDivide
    self error: 'division by zero'!


"mathematics"
Number method!
floor
	"Answer the integer nearest the receiver toward negative infinity."

	| truncation |

	truncation := self truncated.
	self >= 0 ifTrue: [^truncation].
	self = truncation
		ifTrue: [^truncation]
		ifFalse: [^truncation - 1]!

Number method!
exp
    ^ self asFloat exp!

Number method!
ln
    ^ self asFloat ln!

Number method!
log: aNumber
	^ self ln / aNumber ln!

Number method!
floorLog: radix
    self notYetImplemented!

Number method!
raisedTo: aNumber
    self notYetImplemented!

Number method!
raisedToInteger: anInteger
    self notYetImplemented!

Number method!
sqrt
    ^ self asFloat sqrt!

Number method!
sin
    ^ self asFloat sin!

Number method!
cos
    ^ self asFloat cos!

Number method!
tan
    ^ self asFloat tan!

Number method!
arcSin
    ^ self asFloat arcSin!

Number method!
arcCos
    ^ self asFloat arcCos!

Number method!
arcTan
    ^ self asFloat arcTan!

Number method!
squared
    "Answer the square of the receiver"
    ^ self * self!

Number method!
raisedToInteger: anInteger
	| count result |

	self = 0
		ifTrue: [ ^ self zero ].
	(self = 1) | (anInteger = 0)
		ifTrue: [ ^ self unity ].

	anInteger isNegative
		ifTrue: [ ^ (self raisedToInteger: anInteger abs) reciprocal ].

	result := 1.
	count := 0.
	[ (count := count + 1) <= anInteger ]
		whileTrue: [ result := result * self ].

	^ result!

Number method!
raisedTo: aNumber

	self = 0
		ifTrue: [ ^ self zero ].
	(self = 1) | (aNumber = 0)
		ifTrue: [ ^ self unity ].

	(aNumber isFloat) & (self isNegative)
		ifTrue: [ self error: 'raising a negative number to non-integral exponent'].

	aNumber isFloat
		ifTrue: [ ^ (aNumber * self ln) exp ].

	^ self raisedToInteger: aNumber!

"printing"

Number method!
printStringBase: anInteger
    "Answer a String representation of the receiver"
    | stream |
    stream := WriteStream on: (String new: 20).
    self printOn: stream base: anInteger.
    ^ stream contents!

Number method!
printOn: aStream base: anInteger
	self subclassResponsibility!

Number method!
printOn: aStream
	self printOn: aStream base: 10!