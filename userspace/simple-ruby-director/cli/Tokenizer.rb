require "shellwords"

class TokenStream
    def initialize(text)
        @text = text
        tokenize
    end
    
    def pop
        @tokens.shift()
    end
    
    def peak
        @tokens[0]
    end
    
    def push(token)
        @tokens.unshift(token)
    end
    
    def empty?
        return @tokens.empty?
    end
    
private
    def tokenize
        @tokens = Shellwords.shellwords(@text)
    end
end