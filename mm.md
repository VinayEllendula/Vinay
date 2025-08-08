# SageMaker Pipelines Beginner Guide

## 1. What is SageMaker Pipelines?
SageMaker Pipelines is a CI/CD service for machine learning. It automates:
- **Data Preparation**
- **Model Training**
- **Model Evaluation**
- **Model Registration**
- **Deployment**

---

## 2. What is Scikit-learn (sklearn)?
Scikit-learn is an open-source Python library for machine learning.

**It provides:**
- Machine learning algorithms (Logistic Regression, Random Forest, etc.)
- Tools for data preprocessing
- Evaluation metrics

**Analogy:**  
If you're cooking food, scikit-learn gives you ready-made recipes (ML algorithms) and tools (preprocessing, evaluation).

---

## 3. What is a SageMaker Estimator?
An **Estimator** in SageMaker:
- Runs your training script on AWS infrastructure
- Takes input data and outputs a trained model

**Analogy:**  
An Estimator is like a chef who takes ingredients (data) and a recipe (script) to produce a dish (trained model).

---

## 4. SageMaker SDK Components

### `Pipeline`
Main object that defines the ML workflow.

### `ProcessingStep`
Runs preprocessing scripts (e.g., data cleaning).

### `TrainingStep`
Runs training jobs.

### `ModelStep`
Registers trained models.

### `ScriptProcessor`
Runs your custom Python script in a container.

### `ProcessingInput` / `ProcessingOutput`
Define input and output locations for processing jobs.

### `SKLearnProcessor`
Runs preprocessing scripts using a scikit-learn environment.

### `ParameterString`
Defines parameters for your pipeline (like dataset path).

### `PipelineSession`
Handles execution of SageMaker Pipelines.

### `SKLearn Estimator`
Runs training scripts in a scikit-learn container.

---

## 5. Example Pipeline Flow

```text
[ raw CSV in S3 ] --> [ preprocessing.py using SKLearnProcessor ]
                            |
                            v
                [ cleaned CSV ] --> [ train.py using SKLearn Estimator ]
                                            |
                                            v
                                 [ Trained model artifact ]
                                            |
                                            v
                            (Optional) Register model in Model Registry
```

---

## 6. Glossary

| Term                | Meaning                                                                 |
|---------------------|-------------------------------------------------------------------------|
| `sklearn`           | Python library for machine learning                                     |
| `Estimator`         | Object that runs your training script on AWS                            |
| `ProcessingStep`    | Step for data preparation                                               |
| `TrainingStep`      | Step for training a model                                               |
| `ModelStep`         | Registers the trained model                                             |
| `SKLearnProcessor`  | Runs preprocessing scripts using scikit-learn                           |
| `SKLearn Estimator` | Runs training jobs using scikit-learn                                   |
| `Pipeline`          | Combines all steps into a single workflow                               |
| `PipelineSession`   | Manages pipeline execution                                              |

---

## 7. Minimal Python Example

```python
from sagemaker.workflow.pipeline import Pipeline
from sagemaker.workflow.steps import ProcessingStep, TrainingStep
from sagemaker.sklearn.processing import SKLearnProcessor
from sagemaker.sklearn.estimator import SKLearn
from sagemaker.workflow.parameters import ParameterString
from sagemaker.workflow.pipeline_context import PipelineSession
import sagemaker

role = sagemaker.get_execution_role()
pipeline_session = PipelineSession()

# Parameter for dataset location
input_data = ParameterString(name="InputDataUrl", default_value="s3://your-bucket/data/train.csv")

# Preprocessing step
processor = SKLearnProcessor(
    framework_version="0.23-1",
    role=role,
    instance_type="ml.m5.xlarge",
    instance_count=1,
    sagemaker_session=pipeline_session
)

step_process = ProcessingStep(
    name="PreprocessData",
    processor=processor,
    inputs=[],
    outputs=[],
    code="preprocessing.py"
)

# Training step
sklearn_estimator = SKLearn(
    entry_point="train.py",
    framework_version="0.23-1",
    instance_type="ml.m5.large",
    role=role,
    sagemaker_session=pipeline_session
)

step_train = TrainingStep(
    name="TrainModel",
    estimator=sklearn_estimator,
    inputs={}
)

# Define pipeline
pipeline = Pipeline(
    name="MySimpleMLPipeline",
    parameters=[input_data],
    steps=[step_process, step_train],
    sagemaker_session=pipeline_session
)

pipeline.upsert(role_arn=role)
execution = pipeline.start()
execution.wait()
```

---

**This guide should help you understand the basics of SageMaker Pipelines, scikit-learn, Estimators, and how to set up a minimal pipeline.**