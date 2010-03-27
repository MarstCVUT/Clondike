# Class representing a session between nodes
class Session
    # Public key of peer
    attr_reader :remoteNode
    # Negotiated secret to be passed by the client when performing "connect" at the kernel socket layer
    attr_reader :authenticationProof
    # Group certificates claimed by peer
    attr_reader :groupCertificates
    # Authorization certificates claimed by peer
    attr_reader :authorizationCertificates
    
    def initialize(remoteNode, authenticationProof)
        @remoteNode = remoteNode
        @authenticationProof = authenticationProof
        @groupCertificates = Set.new
        @authorizationCertificates = Set.new
    end
    
    def addGroupCertificate(groupCertificate)
        @groupCertificates.add(groupCertificate)
    end
    
    def addAuthorizationCertificate(authorizationCertificate)
        @authorizationCertificates.add(authorizationCertificate)
    end    
end