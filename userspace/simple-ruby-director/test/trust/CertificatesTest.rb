require 'test/unit'
require 'trust/Identity'
require 'trust/Permission'
require 'cmdparse'

class CertificatesTest < Test::Unit::TestCase
  def testFailedSaveLoadNodeCertificate
    identity = Identity.create()      
    identity.save("test/trust/ident1")
    certificate = loadCertificate("test/trust/ident1/identity.cert")
    assert !certificate.verifySignature()
    assert certificate.kind_of?(NodeCertificate)
  end
    
  def testSuccessSaveLoadNodeCertificate
    identity = Identity.create()      
    identity.certificate.sign(identity.privateKey)
    identity.save("test/trust/ident1")
    certificate = loadCertificate("test/trust/ident1/identity.cert")
    assert certificate.verifySignature()
    assert certificate.kind_of?(NodeCertificate)
  end

  def testSaveLoadGroupMembershipCertificate
    identity = Identity.create()      
    identity.save("test/trust/ident1")

    identity2 = Identity.create()      
    identity2.save("test/trust/ident2")
      
    groupCert = identity.issueGroupMembershipCertificate("Test", identity2.publicKey)
    groupCert.save("test/trust/ident1/group1.cert")
    
    loadedGroupCert = loadCertificate("test/trust/ident1/group1.cert")
    assert loadedGroupCert.kind_of?(GroupMembershipCertificate)
    assert !loadedGroupCert.verifySignature()
    
    loadedGroupCert.sign(identity.privateKey)
    loadedGroupCert.save("test/trust/ident1/group1.cert")
    
    loadedGroupCert = loadCertificate("test/trust/ident1/group1.cert")    
    assert loadedGroupCert.verifySignature()
  end

  def testSaveLoadAuthorizationCertificate
    identity = Identity.create()      
    identity.save("test/trust/ident1")

    identity2 = Identity.create()      
    identity2.save("test/trust/ident2")
      
    permissions = Set.new
    permissions.add(FileSystemPermission.new("read","/home"))
    permissions.add(FileSystemPermission.new("readwrite","/tmp"))
    cert = identity.issueAuthorizationCertificate(identity2.publicKey, nil, identity.publicKey, "Test", permissions, false)
    cert.save("test/trust/ident1/auth1.cert")
    
    loadedCert = loadCertificate("test/trust/ident1/auth1.cert")
    assert loadedCert.kind_of?(AuthorizationCertificate)
    assert !loadedCert.verifySignature()
    
    loadedCert.sign(identity.privateKey)
    loadedCert.save("test/trust/ident1/auth1.cert")
    
    loadedCert = loadCertificate("test/trust/ident1/auth1.cert")    
    assert loadedCert.verifySignature()
    assert loadedCert.permissions.size == 2
  end
  
end
