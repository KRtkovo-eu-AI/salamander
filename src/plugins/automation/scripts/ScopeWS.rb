# viz https://forum.altap.cz/viewtopic.php?f=6&t=31928

class TestScope
  attr_reader :wscript_obj
  def initialize(wscript_obj)
    @wscript_obj=wscript_obj
  end
  def do
    # Objekt WScript neni zde ve tride k dispozici...
    #WScript.Echo("In Class")
    # ALE m�me na n�j odkaz wscript_obj
    wscript_obj.Echo("In Class") # ji� v pohod� funguje
  end
end

WScript.Echo("GLOBAL")

cls = TestScope.new(WScript)
cls.do
