package android.hardware.security.timestamp;

/**
 * Represents an RFC3161 TimeStampReq structure.
 * See RFC3161, Section 2.4.1 for the definition.
 */
@VintfStability
parcelable TimeStampReq {
    /**
     * The ASN.1 DER encoded TimeStampReq structure.
     */
    byte[] encodedReq;
}
