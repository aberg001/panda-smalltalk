
"testing"

Set method!
includeAll: aCollection
	| index |

	aCollection do: [ :e | self include: e ].

	^ aCollection!

Set method!
includes: anObject
	| index |

	(array at: (self find: anObject)) ifNil: [^ false].

	^ true!

Set method!
occurancesOf: anObject
	(self includes: anObject)
		ifTrue: [^ 1]
		ifFalse: [^ 0]!

"including"

Set method!
add: anObject
	^ self include: anObject!

Set method!
include: anObject
	| index |
	anObject ifNil: [ self error: 'Sets cannot meaningfully contain nil'].

	index := self find: anObject.
	(array at: index) ifNotNil: [^ anObject].
	self at: index include: anObject.
	^ anObject!


"removing"

Set method!
remove: anObject ifAbsent: aBlock
	| index |

	index := self find: anObject.

	(array at: index) ifNil: [^ aBlock value]. 
	
	self removeAtIndex: index.

	^ anObject!

"converting"
Set method!
asSet
	^ self!


"private"

Set method!
find: anObject in: anArray
	| i mask |

	mask := anArray size - 1.

	i := (anObject hash bitAnd: mask) + 1.

	[ | object | 

	  object := anArray at: i.

	  (object = anObject) | (object == nil)
		  ifTrue: [^ i].

	  i := (i + 106720 bitAnd: mask) + 1.

	] repeat!