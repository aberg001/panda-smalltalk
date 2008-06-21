/*
 * st-universe.c
 *
 * Copyright (C) 2008 Vincent Geddes
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
*/

#include "st-types.h"
#include "st-utils.h"
#include "st-object.h"
#include "st-float.h"
#include "st-association.h"
#include "st-method.h"
#include "st-array.h"
#include "st-byte-array.h"
#include "st-word-array.h"
#include "st-float-array.h"
#include "st-small-integer.h"
#include "st-hashed-collection.h"
#include "st-large-integer.h"
#include "st-symbol.h"
#include "st-universe.h"
#include "st-heap-object.h"
#include "st-lexer.h"
#include "st-descriptor.h"
#include "st-compiler.h"
#include "st-object-memory.h"
#include "st-processor.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

st_oop
    st_nil		     = 0,
    st_true		     = 0,
    st_false		     = 0,
    st_symbol_table	     = 0,
    st_smalltalk	     = 0,
    st_undefined_object_class = 0,
    st_metaclass_class       = 0,
    st_behavior_class        = 0,
    st_smi_class	     = 0,
    st_large_integer_class   = 0,
    st_float_class           = 0,
    st_character_class       = 0,
    st_true_class            = 0,
    st_false_class           = 0,
    st_array_class           = 0,
    st_byte_array_class      = 0,
    st_word_array_class      = 0,
    st_float_array_class      = 0,
    st_set_class	     = 0,
    st_dictionary_class      = 0,
    st_association_class     = 0,
    st_string_class          = 0,
    st_symbol_class          = 0,
    st_wide_string_class     = 0,
    st_compiled_method_class = 0,
    st_method_context_class  = 0,
    st_block_context_class   = 0,
    st_selector_doesNotUnderstand   = 0,
    st_selector_mustBeBoolean       = 0,
    st_selector_startupSystem       = 0,
    st_selector_cannotReturn        = 0;


st_oop st_specials[ST_NUM_SPECIALS];


st_oop
st_global_get (const char *name)
{
    return st_dictionary_at (st_smalltalk, st_symbol_new (name));
}

enum
{
    INSTANCE_SIZE_UNDEFINED = 0,
    INSTANCE_SIZE_CLASS = 6,
    INSTANCE_SIZE_METACLASS = 6,
    INSTANCE_SIZE_DICTIONARY = 2,
    INSTANCE_SIZE_SET = 2,
    INSTANCE_SIZE_ASSOCIATION = 2,
};

static st_oop
class_new (st_format format, st_uint instance_size)
{
    st_oop class;

    class = st_space_allocate_chunk (om->fixed_space, ST_TYPE_SIZE (struct st_class));

    /* TODO refactor this initialising */
    ST_POINTER (class)->header = 0 | ST_MARK_TAG;
    st_heap_object_set_format (class, ST_FORMAT_OBJECT);
    st_heap_object_set_marked (class, false);
    st_heap_object_set_hash (class, st_current_hash++);			       
    st_heap_object_class (class) = st_nil;

    ST_BEHAVIOR (class)->format             = st_smi_new (format);
    ST_BEHAVIOR (class)->instance_size      = st_smi_new (instance_size);
    ST_BEHAVIOR (class)->superclass         = st_nil;
    ST_BEHAVIOR (class)->method_dictionary  = st_nil;
    ST_BEHAVIOR (class)->instance_variables = st_nil;

    ST_CLASS (class)->name = st_nil;

    return class;
}

static void
add_global (const char *name, st_oop object)
{
    st_oop symbol;

    // sanity check for symbol interning
    st_assert (st_symbol_new (name) == st_symbol_new (name));

    symbol = st_symbol_new (name);
    st_dictionary_at_put (st_smalltalk, symbol, object);

    // sanity check for dictionary
    st_assert (st_dictionary_at (st_smalltalk, symbol) == object);
}

static void
parse_error (char *message, st_token *token)
{
    fprintf (stderr, "error: %i: %i: %s",
	     st_token_get_line (token), st_token_get_column (token), message);
    exit (1);
}

static void
initialize_class  (const char *name,
		   const char *super_name,
		   st_list    *ivarnames)
{
    st_oop metaclass, class, superclass;

    if (streq (name, "Object") && streq (super_name, "nil")) {

	class = st_dictionary_at (st_smalltalk, st_symbol_new ("Object"));
	st_assert (class != st_nil);

	metaclass = st_object_class (class);
	if (metaclass == st_nil) {
	    metaclass = st_object_new (om->fixed_space, st_metaclass_class);
	    st_heap_object_class (class) =  metaclass;
	}

	ST_BEHAVIOR (class)->superclass     = st_nil;
	ST_BEHAVIOR (class)->instance_size  = st_smi_new (0);
	ST_BEHAVIOR (metaclass)->superclass = st_dictionary_at (st_smalltalk, st_symbol_new ("Class"));

    } else {
	
	superclass = st_global_get (super_name);
	if (superclass == st_nil)
	    st_assert (superclass != st_nil);

	class = st_global_get (name);
	if (class == st_nil)
	    class = class_new (st_smi_value (ST_BEHAVIOR (superclass)->format), 0);

	metaclass = st_heap_object_class (class);
	if (metaclass == st_nil) {
	    metaclass = st_object_new (om->fixed_space, st_metaclass_class);
	    st_heap_object_class (class) = metaclass;
	}

	ST_BEHAVIOR (class)->superclass     = superclass;
	ST_BEHAVIOR (metaclass)->superclass = st_heap_object_class (superclass);

	ST_BEHAVIOR (class)->instance_size = st_smi_new (st_list_length (ivarnames) +
							 st_smi_value (ST_BEHAVIOR (superclass)->instance_size));	
    }

    ST_BEHAVIOR (metaclass)->format             = st_smi_new (ST_FORMAT_OBJECT);
    ST_BEHAVIOR (metaclass)->method_dictionary  = st_dictionary_new ();
    ST_BEHAVIOR (metaclass)->instance_variables = st_nil;
    ST_BEHAVIOR (metaclass)->instance_size      = st_smi_new (INSTANCE_SIZE_CLASS);
    ST_METACLASS (metaclass)->instance_class    = class;

    if (st_list_length (ivarnames) != 0) {
	st_oop names;
	st_uint i = 1;
	names = st_object_new_arrayed (om->fixed_space, st_array_class, st_list_length (ivarnames));
	for (st_list *l = ivarnames; l; l = l->next)
	    st_array_at_put (names, i++, st_symbol_new (l->data));
	ST_BEHAVIOR (class)->instance_variables = names;

    } else {
	ST_BEHAVIOR (class)->instance_variables = st_nil;
    }

    ST_BEHAVIOR (class)->method_dictionary = st_dictionary_new ();
    ST_CLASS (class)->name = st_symbol_new (name);

    st_dictionary_at_put (st_smalltalk, st_symbol_new (name), class);
}


static bool
parse_variable_names (st_lexer *lexer, st_list **varnames)
{
    st_lexer *ivarlexer;
    st_token *token;
    char *names;

    token = st_lexer_next_token (lexer);

    if (st_token_get_type (token) != ST_TOKEN_STRING_CONST)
	return false;

    names = st_strdup (st_token_get_text (token));
    ivarlexer = st_lexer_new (names); /* input valid at this stage */
    token = st_lexer_next_token (ivarlexer);
    
    while (st_token_get_type (token) != ST_TOKEN_EOF) {
	
	if (st_token_get_type (token) != ST_TOKEN_IDENTIFIER)
	    parse_error (NULL, token);

	*varnames = st_list_append (*varnames, st_strdup (st_token_get_text (token)));
	token = st_lexer_next_token (ivarlexer);
    }

    st_lexer_destroy (ivarlexer);

    return true;
}


static void
parse_class (st_lexer *lexer, st_token *token)
{
    char *class_name = NULL;
    char *superclass_name = NULL;

    // 'Class' token
    if (st_token_get_type (token) != ST_TOKEN_IDENTIFIER
	|| !streq (st_token_get_text (token), "Class"))
	parse_error ("expected class definition", token);	

    // `named:' token
    token = st_lexer_next_token (lexer);
    if (st_token_get_type (token) != ST_TOKEN_KEYWORD_SELECTOR
	|| !streq (st_token_get_text (token), "named:"))
	parse_error ("expected 'name:'", token);

    // class name
    token = st_lexer_next_token (lexer);
    if (st_token_get_type (token) == ST_TOKEN_STRING_CONST) {
	class_name = st_strdup (st_token_get_text (token));
    } else {
	parse_error ("expected string literal", token);
    }

    // `superclass:' token
    token = st_lexer_next_token (lexer);	
    if (st_token_get_type (token) != ST_TOKEN_KEYWORD_SELECTOR
	|| !streq (st_token_get_text (token), "superclass:"))
	parse_error ("expected 'superclass:'", token);

    // superclass name
    token = st_lexer_next_token (lexer);
    if (st_token_get_type (token) == ST_TOKEN_STRING_CONST) {
        
	superclass_name = st_strdup (st_token_get_text (token));

    } else {
	parse_error ("expected string literal", token);
    }

    st_list *ivarnames = NULL;

    // 'instanceVariableNames:' keyword selector        
    token = st_lexer_next_token (lexer);
    if (st_token_get_type (token) == ST_TOKEN_KEYWORD_SELECTOR &&
	streq (st_token_get_text (token), "instanceVariableNames:")) {

	parse_variable_names (lexer, &ivarnames);

    } else {
	parse_error (NULL, token);
    }

    token = st_lexer_next_token (lexer);

    initialize_class (class_name, superclass_name, ivarnames);

    st_list_destroy (ivarnames);
    
    return;
}

static void
parse_classes (const char *filename)
{
    char *contents;
    st_lexer *lexer;
    st_token *token;

    if (!st_file_get_contents (filename, &contents)) {
	exit (1);
    }

    lexer = st_lexer_new (contents);
    st_assert (lexer != NULL);
    token = st_lexer_next_token (lexer);

    while (st_token_get_type (token) != ST_TOKEN_EOF) {

	while (st_token_get_type (token) == ST_TOKEN_COMMENT)
	    token = st_lexer_next_token (lexer);

	parse_class (lexer, token);
	token = st_lexer_next_token (lexer);
    }
}

static void
file_in_classes (void)
{
    char *filename;

    parse_classes ("../st/class-defs.st");

    static const char * files[] = 
	{
	    "Stream.st",
	    "PositionableStream.st",
	    "WriteStream.st",
	    "Collection.st",
	    "SequenceableCollection.st",
	    "ArrayedCollection.st",
//	    "HashedCollection.st",
//	    "Set.st",
	    "Array.st",
	    "ByteArray.st",
	    "WordArray.st",
	    "FloatArray.st",
	    "Association.st",
	    "Magnitude.st",
	    "Number.st",
	    "Integer.st",
	    "SmallInteger.st",
	    "LargeInteger.st",
	    "Fraction.st",
	    "Float.st",
	    "Object.st",
	    "UndefinedObject.st",
	    "String.st",
	    "Symbol.st",
	    "ByteString.st",
	    "WideString.st",
	    "Character.st",
	    "UnicodeTables.st",
	    "Behavior.st",
	    "Boolean.st",
	    "True.st",
	    "False.st",
	    "Behavior.st",
	    "ContextPart.st",
	    "BlockContext.st",
	    "Message.st"
	};

    for (st_uint i = 0; i < ST_N_ELEMENTS (files); i++) {
	filename = st_strconcat("..", ST_DIR_SEPARATOR_S, "st", ST_DIR_SEPARATOR_S, files[i], NULL);
	st_compile_file_in (filename);
	st_free (filename);
    }
}

#define NIL_SIZE_OOPS (sizeof (struct st_header) / sizeof (st_oop))

static st_oop
create_nil_object (void)
{
    st_oop nil;

    nil = st_space_allocate_chunk (om->fixed_space, NIL_SIZE_OOPS);

    ST_POINTER (nil)->header = 0 | ST_MARK_TAG;
    st_heap_object_set_marked     (nil, false);
    st_heap_object_set_format     (nil, ST_FORMAT_OBJECT);
    st_heap_object_set_hash       (nil, st_current_hash++);
    st_heap_object_class          (nil) = nil;

    return nil;
}

static void
init_specials (void)
{
    st_specials[ST_SPECIAL_PLUS]      = st_symbol_new ("+");
    st_specials[ST_SPECIAL_MINUS]     = st_symbol_new ("-");
    st_specials[ST_SPECIAL_LT]        = st_symbol_new ("<");
    st_specials[ST_SPECIAL_GT]        = st_symbol_new (">");
    st_specials[ST_SPECIAL_LE]        = st_symbol_new ("<=");
    st_specials[ST_SPECIAL_GE]        = st_symbol_new (">=");
    st_specials[ST_SPECIAL_EQ]        = st_symbol_new ("=");
    st_specials[ST_SPECIAL_NE]        = st_symbol_new ("~=");
    st_specials[ST_SPECIAL_MUL]       = st_symbol_new ("*");
    st_specials[ST_SPECIAL_DIV]       = st_symbol_new ("/");
    st_specials[ST_SPECIAL_MOD]       = st_symbol_new ("\\");
    st_specials[ST_SPECIAL_BITSHIFT]  = st_symbol_new ("bitShift:");
    st_specials[ST_SPECIAL_BITAND]    = st_symbol_new ("bitAnd:");
    st_specials[ST_SPECIAL_BITOR]     = st_symbol_new ("bitOr:");
    st_specials[ST_SPECIAL_BITXOR]    = st_symbol_new ("bitXor:");

    st_specials[ST_SPECIAL_AT]        = st_symbol_new ("at:");
    st_specials[ST_SPECIAL_ATPUT]     = st_symbol_new ("at:put:");
    st_specials[ST_SPECIAL_SIZE]      = st_symbol_new ("size");
    st_specials[ST_SPECIAL_VALUE]     = st_symbol_new ("value");
    st_specials[ST_SPECIAL_VALUE_ARG] = st_symbol_new ("value:");
    st_specials[ST_SPECIAL_IDEQ]      = st_symbol_new ("==");
    st_specials[ST_SPECIAL_CLASS]     = st_symbol_new ("class");
    st_specials[ST_SPECIAL_NEW]       = st_symbol_new ("new");
    st_specials[ST_SPECIAL_NEW_ARG]   = st_symbol_new ("new:");

    st_selector_doesNotUnderstand   = st_symbol_new ("doesNotUnderstand:");
    st_selector_mustBeBoolean       = st_symbol_new ("mustBeBoolean");
    st_selector_startupSystem       = st_symbol_new ("startupSystem");
    st_selector_cannotReturn        = st_symbol_new ("cannotReturn");
}

st_object_memory *om;

void
st_bootstrap_universe (void)
{
    st_oop st_object_class_, st_class_class_;

    om = st_object_memory_new ();

    /* setup format descriptors */
    st_descriptors[ST_FORMAT_OBJECT]        = st_heap_object_descriptor ();
    st_descriptors[ST_FORMAT_ARRAY]         = st_array_descriptor       ();
    st_descriptors[ST_FORMAT_BYTE_ARRAY]    = st_byte_array_descriptor  ();
    st_descriptors[ST_FORMAT_WORD_ARRAY]    = st_word_array_descriptor  ();
    st_descriptors[ST_FORMAT_FLOAT_ARRAY]   = st_float_array_descriptor  ();
    st_descriptors[ST_FORMAT_FLOAT]         = st_float_descriptor       ();
    st_descriptors[ST_FORMAT_LARGE_INTEGER] = st_large_integer_descriptor ();
    st_descriptors[ST_FORMAT_CONTEXT]       = st_context_descriptor ();

    st_nil = create_nil_object ();

    st_object_class_          = class_new (ST_FORMAT_OBJECT, 0); 
    st_undefined_object_class = class_new (ST_FORMAT_OBJECT, 0);
    st_metaclass_class        = class_new (ST_FORMAT_OBJECT, INSTANCE_SIZE_METACLASS);
    st_behavior_class         = class_new (ST_FORMAT_OBJECT, 0);
    st_class_class_           = class_new (ST_FORMAT_OBJECT, INSTANCE_SIZE_CLASS);
    st_smi_class              = class_new (ST_FORMAT_OBJECT, 0);
    st_large_integer_class    = class_new (ST_FORMAT_LARGE_INTEGER, 0);
    st_character_class        = class_new (ST_FORMAT_OBJECT, 0);
    st_true_class             = class_new (ST_FORMAT_OBJECT, 0);
    st_false_class            = class_new (ST_FORMAT_OBJECT, 0);
    st_float_class            = class_new (ST_FORMAT_FLOAT, 0);
    st_array_class            = class_new (ST_FORMAT_ARRAY, 0);
    st_word_array_class       = class_new (ST_FORMAT_WORD_ARRAY, 0);
    st_float_array_class      = class_new (ST_FORMAT_FLOAT_ARRAY, 0);
    st_dictionary_class       = class_new (ST_FORMAT_OBJECT, INSTANCE_SIZE_DICTIONARY);
    st_set_class              = class_new (ST_FORMAT_OBJECT, INSTANCE_SIZE_SET);
    st_byte_array_class       = class_new (ST_FORMAT_BYTE_ARRAY, 0);
    st_symbol_class           = class_new (ST_FORMAT_BYTE_ARRAY, 0);
    st_string_class           = class_new (ST_FORMAT_BYTE_ARRAY, 0);
    st_wide_string_class      = class_new (ST_FORMAT_WORD_ARRAY, 0);
    st_association_class      = class_new (ST_FORMAT_OBJECT, INSTANCE_SIZE_ASSOCIATION);
    st_compiled_method_class  = class_new (ST_FORMAT_OBJECT, 0);
    st_method_context_class   = class_new (ST_FORMAT_CONTEXT, 5);
    st_block_context_class    = class_new (ST_FORMAT_CONTEXT, 7);

    st_heap_object_class (st_nil) = st_undefined_object_class;

    /* special objects */
    st_true         = st_object_new (om->fixed_space, st_true_class);
    st_false        = st_object_new (om->fixed_space, st_false_class);
    st_symbol_table = st_set_new_with_capacity (75);
    st_smalltalk    = st_dictionary_new_with_capacity (75);

    /* add class names to symbol table */
    add_global ("Object", st_object_class_);
    add_global ("UndefinedObject", st_undefined_object_class);
    add_global ("Behavior", st_behavior_class);
    add_global ("Class", st_class_class_);
    add_global ("Metaclass", st_metaclass_class);
    add_global ("SmallInteger", st_smi_class);
    add_global ("LargeInteger", st_large_integer_class);
    add_global ("Character", st_character_class);
    add_global ("True", st_true_class);
    add_global ("False", st_false_class);
    add_global ("Float", st_float_class);
    add_global ("Array", st_array_class);
    add_global ("ByteArray", st_byte_array_class);
    add_global ("WordArray", st_word_array_class);
    add_global ("FloatArray", st_float_array_class);
    add_global ("ByteString", st_string_class);
    add_global ("ByteSymbol", st_symbol_class);
    add_global ("WideString", st_wide_string_class);
    add_global ("Set", st_set_class);
    add_global ("Dictionary", st_dictionary_class);
    add_global ("Association", st_association_class);
    add_global ("CompiledMethod", st_compiled_method_class);
    add_global ("MethodContext", st_method_context_class);
    add_global ("BlockContext", st_block_context_class);

    init_specials ();
    file_in_classes ();

    st_object_memory_add_root (om, st_nil);
    st_object_memory_add_root (om, st_true);
    st_object_memory_add_root (om, st_false);
    st_object_memory_add_root (om, st_smalltalk);
    st_object_memory_add_root (om, st_symbol_table);
}