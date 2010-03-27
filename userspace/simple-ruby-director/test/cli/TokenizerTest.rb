require 'test/unit'
require 'cli/Tokenizer'

class TokenizerTest < Test::Unit::TestCase
  def testTokenization
      tokenizer = TokenStream.new("foo bar")
      assert !tokenizer.empty?
      assert_equal("foo", tokenizer.pop)
      assert !tokenizer.empty?
      assert_equal("bar", tokenizer.pop)      
      assert tokenizer.empty?

      tokenizer.push("foo")      
      assert !tokenizer.empty?
      assert_equal("foo", tokenizer.peak)
      assert_equal("foo", tokenizer.pop)
      assert tokenizer.empty?      
  end
  
  def testTokenizationWithQuotes
      tokenizer = TokenStream.new("foo 'bar war' dar")
      assert !tokenizer.empty?
      assert_equal("foo", tokenizer.pop)
      assert !tokenizer.empty?
      assert_equal("bar war", tokenizer.pop)      
      assert !tokenizer.empty?
      assert_equal("dar", tokenizer.pop)            
      assert tokenizer.empty?      
  end
end
