// Code generated by MockGen. DO NOT EDIT.
// Source: pixielabs.ai/pixielabs/src/vizier/services/metadata/metadatapb (interfaces: MetadataServiceClient)

// Package mock_metadatapb is a generated GoMock package.
package mock_metadatapb

import (
	context "context"
	gomock "github.com/golang/mock/gomock"
	grpc "google.golang.org/grpc"
	metadatapb "pixielabs.ai/pixielabs/src/vizier/services/metadata/metadatapb"
	reflect "reflect"
)

// MockMetadataServiceClient is a mock of MetadataServiceClient interface
type MockMetadataServiceClient struct {
	ctrl     *gomock.Controller
	recorder *MockMetadataServiceClientMockRecorder
}

// MockMetadataServiceClientMockRecorder is the mock recorder for MockMetadataServiceClient
type MockMetadataServiceClientMockRecorder struct {
	mock *MockMetadataServiceClient
}

// NewMockMetadataServiceClient creates a new mock instance
func NewMockMetadataServiceClient(ctrl *gomock.Controller) *MockMetadataServiceClient {
	mock := &MockMetadataServiceClient{ctrl: ctrl}
	mock.recorder = &MockMetadataServiceClientMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use
func (m *MockMetadataServiceClient) EXPECT() *MockMetadataServiceClientMockRecorder {
	return m.recorder
}

// GetAgentInfo mocks base method
func (m *MockMetadataServiceClient) GetAgentInfo(arg0 context.Context, arg1 *metadatapb.AgentInfoRequest, arg2 ...grpc.CallOption) (*metadatapb.AgentInfoResponse, error) {
	varargs := []interface{}{arg0, arg1}
	for _, a := range arg2 {
		varargs = append(varargs, a)
	}
	ret := m.ctrl.Call(m, "GetAgentInfo", varargs...)
	ret0, _ := ret[0].(*metadatapb.AgentInfoResponse)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetAgentInfo indicates an expected call of GetAgentInfo
func (mr *MockMetadataServiceClientMockRecorder) GetAgentInfo(arg0, arg1 interface{}, arg2 ...interface{}) *gomock.Call {
	varargs := append([]interface{}{arg0, arg1}, arg2...)
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetAgentInfo", reflect.TypeOf((*MockMetadataServiceClient)(nil).GetAgentInfo), varargs...)
}

// GetSchemaByAgent mocks base method
func (m *MockMetadataServiceClient) GetSchemaByAgent(arg0 context.Context, arg1 *metadatapb.SchemaByAgentRequest, arg2 ...grpc.CallOption) (*metadatapb.SchemaByAgentResponse, error) {
	varargs := []interface{}{arg0, arg1}
	for _, a := range arg2 {
		varargs = append(varargs, a)
	}
	ret := m.ctrl.Call(m, "GetSchemaByAgent", varargs...)
	ret0, _ := ret[0].(*metadatapb.SchemaByAgentResponse)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetSchemaByAgent indicates an expected call of GetSchemaByAgent
func (mr *MockMetadataServiceClientMockRecorder) GetSchemaByAgent(arg0, arg1 interface{}, arg2 ...interface{}) *gomock.Call {
	varargs := append([]interface{}{arg0, arg1}, arg2...)
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetSchemaByAgent", reflect.TypeOf((*MockMetadataServiceClient)(nil).GetSchemaByAgent), varargs...)
}

// GetSchemas mocks base method
func (m *MockMetadataServiceClient) GetSchemas(arg0 context.Context, arg1 *metadatapb.SchemaRequest, arg2 ...grpc.CallOption) (*metadatapb.SchemaResponse, error) {
	varargs := []interface{}{arg0, arg1}
	for _, a := range arg2 {
		varargs = append(varargs, a)
	}
	ret := m.ctrl.Call(m, "GetSchemas", varargs...)
	ret0, _ := ret[0].(*metadatapb.SchemaResponse)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetSchemas indicates an expected call of GetSchemas
func (mr *MockMetadataServiceClientMockRecorder) GetSchemas(arg0, arg1 interface{}, arg2 ...interface{}) *gomock.Call {
	varargs := append([]interface{}{arg0, arg1}, arg2...)
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetSchemas", reflect.TypeOf((*MockMetadataServiceClient)(nil).GetSchemas), varargs...)
}
