require 'test/unit'
require 'trust/Identity'

class IdentityTest < Test::Unit::TestCase
  def testBasicIdentity
    identity = Identity.create()      
    assert !identity.certificate.verifySignature()
    identity.certificate.sign(identity.privateKey)
    assert identity.certificate.verifySignature()
  end
end
