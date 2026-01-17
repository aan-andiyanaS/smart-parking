package services

import (
	"context"
	"fmt"
	"io"

	"github.com/minio/minio-go/v7"
	"github.com/minio/minio-go/v7/pkg/credentials"
)

// MinIOStorage handles file uploads to MinIO/S3
type MinIOStorage struct {
	client   *minio.Client
	bucket   string
	endpoint string
	useSSL   bool
}

// NewMinIOStorage creates a new MinIO storage client
func NewMinIOStorage(endpoint, accessKey, secretKey, bucket string, useSSL bool) (*MinIOStorage, error) {
	client, err := minio.New(endpoint, &minio.Options{
		Creds:  credentials.NewStaticV4(accessKey, secretKey, ""),
		Secure: useSSL,
	})
	if err != nil {
		return nil, fmt.Errorf("failed to create MinIO client: %w", err)
	}

	storage := &MinIOStorage{
		client:   client,
		bucket:   bucket,
		endpoint: endpoint,
		useSSL:   useSSL,
	}

	// Ensure bucket exists
	ctx := context.Background()
	exists, err := client.BucketExists(ctx, bucket)
	if err != nil {
		return nil, fmt.Errorf("failed to check bucket: %w", err)
	}

	if !exists {
		err = client.MakeBucket(ctx, bucket, minio.MakeBucketOptions{})
		if err != nil {
			return nil, fmt.Errorf("failed to create bucket: %w", err)
		}

		// Set bucket policy to public read
		policy := fmt.Sprintf(`{
			"Version": "2012-10-17",
			"Statement": [
				{
					"Effect": "Allow",
					"Principal": "*",
					"Action": ["s3:GetObject"],
					"Resource": ["arn:aws:s3:::%s/*"]
				}
			]
		}`, bucket)

		err = client.SetBucketPolicy(ctx, bucket, policy)
		if err != nil {
			// Non-fatal, just log
			fmt.Printf("Warning: Failed to set bucket policy: %v\n", err)
		}
	}

	return storage, nil
}

// Upload uploads a file to MinIO and returns the public URL
func (s *MinIOStorage) Upload(ctx context.Context, reader io.Reader, objectName string, size int64, contentType string) (string, error) {
	if contentType == "" {
		contentType = "image/jpeg"
	}

	_, err := s.client.PutObject(ctx, s.bucket, objectName, reader, size, minio.PutObjectOptions{
		ContentType: contentType,
	})
	if err != nil {
		return "", fmt.Errorf("failed to upload object: %w", err)
	}

	// Construct public URL
	protocol := "http"
	if s.useSSL {
		protocol = "https"
	}

	url := fmt.Sprintf("%s://%s/%s/%s", protocol, s.endpoint, s.bucket, objectName)
	return url, nil
}

// Delete removes a file from MinIO
func (s *MinIOStorage) Delete(ctx context.Context, objectName string) error {
	return s.client.RemoveObject(ctx, s.bucket, objectName, minio.RemoveObjectOptions{})
}

// GetURL returns the public URL for an object
func (s *MinIOStorage) GetURL(objectName string) string {
	protocol := "http"
	if s.useSSL {
		protocol = "https"
	}
	return fmt.Sprintf("%s://%s/%s/%s", protocol, s.endpoint, s.bucket, objectName)
}
