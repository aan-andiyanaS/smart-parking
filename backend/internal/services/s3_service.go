package services

import (
	"context"
	"fmt"
	"io"
	"log"
	"os"

	"github.com/aws/aws-sdk-go-v2/aws"
	"github.com/aws/aws-sdk-go-v2/config"
	"github.com/aws/aws-sdk-go-v2/credentials"
	"github.com/aws/aws-sdk-go-v2/service/s3"
)

// S3Storage handles AWS S3 operations
type S3Storage struct {
	client *s3.Client
	bucket string
	region string
}

// NewS3Storage creates a new S3 storage client
func NewS3Storage(region, accessKey, secretKey, bucket string) (*S3Storage, error) {
	// Create credentials
	creds := credentials.NewStaticCredentialsProvider(accessKey, secretKey, "")

	// Load AWS config
	cfg, err := config.LoadDefaultConfig(context.Background(),
		config.WithRegion(region),
		config.WithCredentialsProvider(creds),
	)
	if err != nil {
		return nil, fmt.Errorf("failed to load AWS config: %w", err)
	}

	client := s3.NewFromConfig(cfg)

	storage := &S3Storage{
		client: client,
		bucket: bucket,
		region: region,
	}

	log.Printf("✅ Connected to AWS S3 bucket: %s in region: %s", bucket, region)
	return storage, nil
}

// Upload uploads a file to S3 bucket
func (s *S3Storage) Upload(ctx context.Context, file io.Reader, key string, size int64, contentType string) (string, error) {
	if contentType == "" {
		contentType = "application/octet-stream"
	}

	input := &s3.PutObjectInput{
		Bucket:        aws.String(s.bucket),
		Key:           aws.String(key),
		Body:          file,
		ContentLength: aws.Int64(size),
		ContentType:   aws.String(contentType),
		ACL:           "public-read",
	}

	_, err := s.client.PutObject(ctx, input)
	if err != nil {
		return "", fmt.Errorf("failed to upload to S3: %w", err)
	}

	// Return public URL
	url := fmt.Sprintf("https://%s.s3.%s.amazonaws.com/%s", s.bucket, s.region, key)
	return url, nil
}

// Delete removes a file from S3 bucket
func (s *S3Storage) Delete(ctx context.Context, key string) error {
	input := &s3.DeleteObjectInput{
		Bucket: aws.String(s.bucket),
		Key:    aws.String(key),
	}

	_, err := s.client.DeleteObject(ctx, input)
	if err != nil {
		return fmt.Errorf("failed to delete from S3: %w", err)
	}

	return nil
}

// GetURL returns the public URL for an object
func (s *S3Storage) GetURL(key string) string {
	return fmt.Sprintf("https://%s.s3.%s.amazonaws.com/%s", s.bucket, s.region, key)
}

// IsUsingAWSS3 checks if we should use AWS S3 instead of MinIO
func IsUsingAWSS3() bool {
	return os.Getenv("USE_AWS_S3") == "true"
}
