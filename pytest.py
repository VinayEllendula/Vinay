import pytest

# Mock function to simulate LLM tool calling response
def mock_llm_response(prompt: str):
    """
    This function simulates what the LLM should return for given prompts.
    In your real setup, replace this with your actual LLM call.
    """
    if "fund holdings" in prompt.lower():
        return {"name": "get_fund_holdings", "arguments": {"fund_name": "Vanguard 500 Index Fund"}}
    elif "etf holdings" in prompt.lower():
        return {"name": "get_etf_holdings", "arguments": {"etf_symbol": "VOO"}}
    elif "fund performance" in prompt.lower():
        return {"name": "get_fund_performance", "arguments": {"fund_name": "Vanguard Total Stock Market Fund"}}
    elif "compare" in prompt.lower():
        return {"name": "compare_funds", "arguments": {"fund1": "VFIAX", "fund2": "VTSAX"}}
    else:
        return {"name": "unknown", "arguments": {}}


def test_fund_holdings():
    prompt = "Show me the fund holdings for Vanguard 500 Index Fund"
    expected = {"name": "get_fund_holdings", "arguments": {"fund_name": "Vanguard 500 Index Fund"}}
    response = mock_llm_response(prompt)
    assert response["name"] == expected["name"]
    assert response["arguments"] == expected["arguments"]


def test_etf_holdings():
    prompt = "What are the ETF holdings of VOO?"
    expected = {"name": "get_etf_holdings", "arguments": {"etf_symbol": "VOO"}}
    response = mock_llm_response(prompt)
    assert response["name"] == expected["name"]
    assert response["arguments"] == expected["arguments"]


def test_fund_performance():
    prompt = "Give me the performance details of Vanguard Total Stock Market Fund"
    expected = {"name": "get_fund_performance", "arguments": {"fund_name": "Vanguard Total Stock Market Fund"}}
    response = mock_llm_response(prompt)
    assert response["name"] == expected["name"]
    assert response["arguments"] == expected["arguments"]


def test_compare_funds():
    prompt = "Compare VFIAX with VTSAX"
    expected = {"name": "compare_funds", "arguments": {"fund1": "VFIAX", "fund2": "VTSAX"}}
    response = mock_llm_response(prompt)
    assert response["name"] == expected["name"]
    assert response["arguments"] == expected["arguments"]