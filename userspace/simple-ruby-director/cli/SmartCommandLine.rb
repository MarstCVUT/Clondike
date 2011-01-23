require 'curses'

class LineText
	attr_reader :linePosition

	def initialize()
		reset()
	end

	def addChar(c)
		if ( @linePosition == @currentLineContent.length )
			@currentLineContent = @currentLineContent + c
		else
			@currentLineContent[@linePosition] = c
		end
		@linePosition += 1;
	end

	def delChar
		return false if @linePosition <= 0		

		if ( @linePosition == @currentLineContent.length )
			@currentLineContent = @currentLineContent[0, @currentLineContent.length-1]
		else
			@currentLineContent.slice!(@linePosition - 1)
		end

		@linePosition -= 1;

		return true
	end

	def scrollLeft()
		if ( @linePosition > 0 )
			@linePosition -= 1
			return true
		end

		return false
	end

	def scrollRight()
		#puts "#{@linePosition < @currentLineContent.length} .. #{@currentLineContent.length}"
		if ( @linePosition < @currentLineContent.length )
			@linePosition += 1
			return true
		end

		return false
	end

	def scrollStart()
		@linePosition = 0
	end

	def scrollEnd()
		@linePosition = @currentLineContent.length
	end

	def reset()
		@currentLineContent = ""
		@linePosition = 0
	end

	def resetText(newText)
		raise "No nil text please" if !newText
		@currentLineContent = newText
		@linePosition = newText.length
	end

	def text()
		return @currentLineContent
	end
end

class CommandHistory
	MAX_HISTORY = 500

	def initialize(persistFile = nil)
		@history = []
		@historyIndex = 0
		@persistFile = persistFile
		load()
	end
		
	def addCommand(command)
		return if ( @history.size > 0 && @history[0] == command ) # Do not add duplicate command
		return if ( command.strip.length == 0 ) # Ignore empty commands

		@history.unshift(command)
		if ( @history.size > MAX_HISTORY )
			@history.pop()
		end

		save()
	end

	def resetIndex()
		@historyIndex = 0
	end

	def nextCommand()
		if ( @historyIndex > 1 )
			@historyIndex -= 1
			return @history[@historyIndex - 1]
		end

		return nil
	end

	def previousCommand()
		if ( @historyIndex < @history.size() )
			@historyIndex += 1
			return @history[@historyIndex - 1]
		end

		return nil
	end
private
	def save()
		return if !@persistFile

		output = File.new(@persistFile, "w")
		@history.each { |line|
			output.puts line
		}		
		output.close  		
	end

	def load()
        	return if !@persistFile
		return if !File.exists?(@persistFile)

		File.foreach(@persistFile) { |line|
			# Get rid of line break!
			@history << line[0, line.length-1]
		}
	end
end

class SmartCommandLine
  def initialize(prompt, historyFile, lineInterpreter)
	@prompt = prompt
	@lineInterpreter = lineInterpreter
	@lineText = LineText.new
	@history = CommandHistory.new(historyFile)
	initCurses()
  end

  # Blocking method, withing for input and interpereting it
  def run     
	begin
		Curses.timeout=-1
		loop do
			c = Curses.getch
			case c
				#when ?Q, ?q    :  break
				when Curses::Key::ENTER, 10 : onEnter()
				when Curses::Key::BACKSPACE, 127 : onBackspace()
				when Curses::Key::LEFT :  onLeft()
				when Curses::Key::RIGHT, 261 : onRight()
				when Curses::Key::HOME : onHome()
				when Curses::Key::END : onEnd()
				when Curses::Key::UP : onUp()
				when Curses::Key::DOWN : onDown()
			else
				#puts "#{c}" if ( c < 10000 ) 
				writeChar(c)
			end
		end
	ensure
		Curses.close_screen
	end
  end
  
private
  def isPrintableChar(c)
	return (c >= 32 && c <= 125)
	#return (c <= ?z && c >= ?a) || (c <= ?Z && c >= ?A) || (c <= ?9 && c >= ?0) || (c <= 47 && c >= 32)  || (c <= 64 && c >= 58)
  end

  def onLeft()
	if ( @lineText.scrollLeft )
		scrollCursorLeft()
	end
  end

  def onRight()
	if ( @lineText.scrollRight )
		scrollCursorRight()
	end
  end

  def onUp()
	previousCommand = @history.previousCommand()
	if ( previousCommand )
		deleteAllText()
		@lineText.resetText(previousCommand)
		rewriteText(false)
	end
  end

  def onDown()
	nextCommand = @history.nextCommand()
	if ( nextCommand )
		deleteAllText()
		@lineText.resetText(nextCommand)
		rewriteText(false)
	end
  end

  def onBackspace()
	if ( @lineText.delChar ) 
		currentPosition = @lineText.linePosition		
		scrollCursorLeft(); # Compensate one deleted char
		rewriteText() # Rewrite old text
		@screen.addstr(" "); scrollCursorLeft(); # To ensure last char deletion
		(@lineText.text.length - currentPosition).times { scrollCursorLeft(); } # Get back to original position
	end
  end
  
  def onEnter
	@screen.scrl(1); @screen.setpos(@screen.cury, 0); 
	if ( @lineInterpreter ) 
		result = interpretCommand(@lineText.text)
	else
		result = "No line interpreter available"
	end
	@history.addCommand(@lineText.text)
	@history.resetIndex()
	@screen.addstr(result)
	@screen.scrl(1); @screen.setpos(@screen.cury, 0); @screen.addstr(@prompt)
	@lineText.reset()
  end
  
  def interpretCommand(command)
    return @lineInterpreter.interpret(command)
  end

  def onHome()	
	scrollToLineStart()
	@lineText.scrollStart()
  end

  def onEnd()
	scrollToLineEnd()
	@lineText.scrollEnd()
  end

  def scrollCursorLeft()
	if ( @screen.curx > 0 )
		@screen.setpos(@screen.cury, @screen.curx-1);
	else
		@screen.setpos(@screen.cury-1, @screen.maxx-1);
	end
  end

  def scrollCursorRight()
	#puts ":::::: #{@screen.cury}, #{@screen.curx} "
	if ( @screen.curx < (@screen.maxx - 1) )
		@screen.setpos(@screen.cury, @screen.curx+1);
	else
		@screen.setpos(@screen.cury+1, 0);
	end	
  end

  def scrollToLineStart()
	@lineText.linePosition.times { scrollCursorLeft() }
  end

  def scrollToLineEnd()
	(@lineText.text.length - @lineText.linePosition).times { scrollCursorRight() }
  end

  # Deletes all text from command line and scrolls to line beginning
  def deleteAllText()
	scrollToLineStart()	
	@lineText.text.each_byte { |c|		
		@screen.addch(?\s) 
	}
	@lineText.text.length.times { scrollCursorLeft() }
  end

  def rewriteText(scrollToStart = true)
	scrollToLineStart() if scrollToStart
	@lineText.text.each_byte { |c|
		@screen.addch(c) 
	}
  end

  def writeChar(c)
	if ( isPrintableChar(c) ) 
		@screen.addch(c) 
		@lineText.addChar(c.chr)
	end
  end

  def initCurses
	Curses.init_screen
	#Curses.cbreak
	Curses.noecho
	Curses.stdscr.keypad(true)
	@screen = Curses.stdscr #.subwin(27, 81, 0, 0)
	@screen.scrollok(true)
	@screen.setpos(@screen.maxy-1,0)
	@screen.addstr(@prompt)
  end 
end

#commandLine = SmartCommandLine.new("Director> ", "history.tmp", nil)
#commandLine.run