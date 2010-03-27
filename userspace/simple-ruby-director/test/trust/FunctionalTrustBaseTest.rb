# 
# To change this template, choose Tools | Templates
# and open the template in the editor.
 

$:.unshift File.join(File.dirname(__FILE__),'..','lib')

require 'test/unit'
require 'fileutils'
require 'trust/Identity.rb'
require 'trust/TrustManagement.rb'
require 'trust/Permission.rb'

class FunctionalTrustBaseTest < Test::Unit::TestCase
    undef_method :default_test

protected
  def generateIdentities
      identity1 = Identity.create('test/trust/identities/identity1')      
      identity1.save()      
      identity2 = Identity.create('test/trust/identities/identity2')      
      identity2.save()      
      identity3 = Identity.create('test/trust/identities/identity3')      
      identity3.save()
      identity4 = Identity.create('test/trust/identities/identity4') 
      identity4.save()
      
      [identity1, identity2, identity3, identity4]
  end
  
  def prepareDirectories      
      FileUtils::mkdir 'test/trust/identities' if ( !File.exists?('test/trust/identities'))
      FileUtils::remove_entry_secure 'test/trust/identities/identity1' if ( File.exists?('test/trust/identities/identity1'))
      FileUtils::mkdir 'test/trust/identities/identity1'
      FileUtils::remove_entry_secure 'test/trust/identities/identity2' if ( File.exists?('test/trust/identities/identity2'))
      FileUtils::mkdir 'test/trust/identities/identity2'
      FileUtils::remove_entry_secure 'test/trust/identities/identity3' if ( File.exists?('test/trust/identities/identity3'))
      FileUtils::mkdir 'test/trust/identities/identity3'
      FileUtils::remove_entry_secure 'test/trust/identities/identity4' if ( File.exists?('test/trust/identities/identity4'))
      FileUtils::mkdir 'test/trust/identities/identity4'      
  end
end
